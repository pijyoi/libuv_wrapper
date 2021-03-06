#ifndef UVPP_TCP_HPP
#define UVPP_TCP_HPP

#include "uvpp_stream.hpp"

namespace uvpp
{
    class Tcp;

    struct TcpImpl
    {
        uv_tcp_t handle;
        DataReadCallback callback;
        MemPool<65536> mempool;

        ConnectionCallback<Tcp> connection_cb;
        StatusCallback connect_cb;
    };

    class Tcp : public Stream<TcpImpl>
    {
    public:
        Tcp(uv_loop_t *uvloop, unsigned int flags=AF_UNSPEC) {
            // Loop can decay to uv_loop_t*
            uv_tcp_init_ex(uvloop, phandle(), flags);
        }

        int bind(const sockaddr *saddr, unsigned int flags=0) {
            int rc = uv_tcp_bind(phandle(), saddr, flags);
            assert(rc==0 || print_error(rc));
            return rc;
        }

        int getsockname(sockaddr *saddr, int namelen) {
            int rc = uv_tcp_getsockname(phandle(), saddr, &namelen);
            assert(rc==0 || print_error(rc));
            return rc;
        }

        int getpeername(sockaddr *saddr, int namelen) {
            int rc = uv_tcp_getpeername(phandle(), saddr, &namelen);
            assert(rc==0 || print_error(rc));
            return rc;
        }

        int listen(ConnectionCallback<Tcp> connection_cb) {
            pimpl->connection_cb = connection_cb;
            int rc = uv_listen(as_stream(), 0, [](uv_stream_t *server, int status){
                if (status < 0) {
                    print_error(status);
                    return;
                }
                auto pimpl = static_cast<Impl*>(server->data);
                // uv_accept will fail if socket has been created, thus the use of AF_UNSPEC
                Tcp client(server->loop, AF_UNSPEC);
                int rc = uv_accept(server, client.as_stream());
                // documentation guarantees success...
                assert(rc==0 || print_error(rc));
                pimpl->connection_cb(std::move(client));
            });
            assert(rc==0 || print_error(rc));
            return rc;
        }

        int connect(const sockaddr *saddr, StatusCallback connect_cb) {
            pimpl->connect_cb = connect_cb;
            auto req = new uv_connect_t();
            req->data = pimpl;
            int rc = uv_tcp_connect(req, phandle(), saddr,
                [](uv_connect_t *req, int status){
                    auto pimpl = static_cast<Impl*>(req->data);
                    delete req;
                    pimpl->connect_cb(status);
            });
            assert(rc==0 || print_error(rc));
            return rc;
        }
    };
}

#endif
