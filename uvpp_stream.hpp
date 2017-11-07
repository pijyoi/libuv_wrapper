#ifndef UVPP_STREAM_HPP
#define UVPP_STREAM_HPP

#include <string.h>

#include "uvpp.hpp"
#include "uvpp_util.hpp"

namespace uvpp
{
    typedef std::function<void(char *, int)> DataReadCallback;

    template <typename TypeImpl>
    class Stream : public BaseHandle<TypeImpl>
    {
    public:
        typedef TypeImpl Impl;

        int read_start()
        {
            int rc = uv_read_start(as_stream(),
                [](uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf){
                    auto pimpl = static_cast<Impl*>(handle->data);
                    buf->base = pimpl->mempool.get();
                    buf->len = pimpl->mempool.buflen();
                },
                [](uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf){
                    auto pimpl = static_cast<Impl*>(stream->data);
                    if (nread==0) {
                        // EAGAIN: don't pass this to user
                    }
                    else if (nread < 0) {
                        pimpl->callback(buf->base, nread);
                    }
                    else {
                        pimpl->callback(buf->base, nread);
                    }

                    if (buf->base)
                        pimpl->mempool.put(buf->base);
                });
            assert(rc==0 || print_error(rc));
            return rc;
        }

        int read_stop()
        {
            int rc = uv_read_stop(as_stream());
            assert(rc==0 || print_error(rc));
            return rc;
        }

        int write(const char *buf, int len)
        {
            // use multiple requests for large writes
            while (len > 0) {
                auto& mempool = this->pimpl->mempool;

                char *mem = mempool.get();
                auto req = (uv_write_t*)mem;
                char *payload = mem + sizeof(*req);

                int chunk = std::min(mempool.bufsize(), len);
                memcpy(payload, buf, chunk);

                buf += chunk;
                len -= chunk;

                uv_buf_t uvbuf = uv_buf_init(payload, chunk);

                uv_write(req, as_stream(), &uvbuf, 1,
                [](uv_write_t *req, int status) {
                    auto pimpl = static_cast<Impl*>(req->handle->data);
                    pimpl->mempool.put((char*)req);
                });
            }
            return 0;
        }

    protected:
        uv_stream_t *as_stream() { return (uv_stream_t *)&this->pimpl->handle; }
    };
}

#endif
