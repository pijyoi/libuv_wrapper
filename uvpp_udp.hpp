#ifndef UVPP_UDP_HPP
#define UVPP_UDP_HPP

#include <string>
#include <stack>

#include <string.h>

#include "uvpp.hpp"

namespace uvpp
{
    typedef std::function<void(char *, int, const struct sockaddr*)> DataRecvCallback;

    struct UdpImpl
    {
        uv_udp_t handle;
        DataRecvCallback callback;
        std::stack<void*> mempool;

        ~UdpImpl()
        {
            while (!mempool.empty()) {
                auto ptr = mempool.top();
                mempool.pop();
                free(ptr);
            }
        }

        void alloc_callback(size_t suggested_size, uv_buf_t *buf) {
            size_t nbytes = 65536;
            if (mempool.empty()) {
                buf->base = (char *)malloc(nbytes);
                buf->len = nbytes;
            }
            else {
                buf->base = (char *)mempool.top();
                buf->len = nbytes;
                mempool.pop();
            }
        }

        void recv_callback(ssize_t nread, const uv_buf_t *buf,
            const struct sockaddr *addr, unsigned flags) {
            if (nread==0 && addr==NULL) {
                // EAGAIN: don't pass this to user
            }
            else if (nread < 0) {
                callback(buf->base, nread, addr);
            }
            else {
                callback(buf->base, nread, addr);
            }

            if (buf->base)
                mempool.push(buf->base);
        }
    };

    class Udp : public BaseHandle<UdpImpl>
    {
    public:
        Udp(Loop& uvloop)
        {
            uv_udp_init_ex(uvloop, phandle(), AF_INET);
        }

        int bind(const struct sockaddr_in& saddr, unsigned int flags=0) {
            int rc = uv_udp_bind(phandle(), (struct sockaddr*)&saddr, flags);
            assert(rc==0 || print_error(rc));
            return rc;
        }

        int getsockname(struct sockaddr_in& saddr) {
            int namelen = sizeof(saddr);
            return uv_udp_getsockname(phandle(), (struct sockaddr*)&saddr, &namelen);
        }

        int set_membership(const std::string& mcast_addr, const std::string& iface_addr,
                uv_membership membership) {
            int rc = uv_udp_set_membership(phandle(), mcast_addr.c_str(),
                iface_addr.c_str(), membership);
            assert(rc==0 || print_error(rc));
            return rc;
        }

        int set_multicast_loop(bool on) {
            int rc = uv_udp_set_multicast_loop(phandle(), on);
            assert(rc==0 || print_error(rc));
            return rc;
        }

        int set_multicast_interface(const std::string& iface_addr) {
            int rc = uv_udp_set_multicast_interface(phandle(), iface_addr.c_str());
            assert(rc==0 || print_error(rc));
            return rc;
        }

        int join(const std::string& mcast_addr, const std::string& iface_addr) {
            return set_membership(mcast_addr, iface_addr, UV_JOIN_GROUP);
        }

        int recv_buffer_size(int size=0) {
            int value = size;
            uv_recv_buffer_size(as_base_handle(), &value);
            return size==0 ? value : recv_buffer_size();
        }

        int send_buffer_size(int size=0) {
            int value = size;
            uv_send_buffer_size(as_base_handle(), &value);
            return size==0 ? value : send_buffer_size();
        }

        void set_callback(DataRecvCallback cb)
        {
            pimpl->callback = cb;
        }

        void start()
        {
            int rc = uv_udp_recv_start(phandle(),
                [](uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf){
                    auto pimpl = static_cast<Impl*>(handle->data);
                    pimpl->alloc_callback(suggested_size, buf);
                },
                [](uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf,
                    const struct sockaddr *addr, unsigned flags){
                    auto pimpl = static_cast<Impl*>(handle->data);
                    pimpl->recv_callback(nread, buf, addr, flags);
                });
            assert(rc==0 || print_error(rc));
        }

        void stop()
        {
            int rc = uv_udp_recv_stop(phandle());
            assert(rc==0 || print_error(rc));
        }

        int try_send(const char *buf, int len, const struct sockaddr_in& saddr) {
            uv_buf_t uvbuf = uv_buf_init(const_cast<char*>(buf), len);
            return uv_udp_try_send(phandle(), &uvbuf, 1,
                (const struct sockaddr *)&saddr);
        }

        int send(const char *buf, int len, const struct sockaddr_in& saddr) {
            // our mempool only returns 64KB buffers
            if (sizeof(uv_udp_send_t) + len > 65536)
                return UV_EMSGSIZE;

            // get buf from mempool
            uv_buf_t uvbuf;
            pimpl->alloc_callback(65536, &uvbuf);

            uv_udp_send_t *req = (uv_udp_send_t*)uvbuf.base;
            char *data = (char *)(req + 1);
            memcpy(data, buf, len);
            uvbuf = uv_buf_init(data, len);

            int rc = uv_udp_send(req, phandle(), &uvbuf, 1,
                (const struct sockaddr *)&saddr, [](uv_udp_send_t *req, int status) {
                auto pimpl = static_cast<Impl*>(req->handle->data);
                pimpl->mempool.push(req);
            });
            return rc;
        }
    };
}

#endif

