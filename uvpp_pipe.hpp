#ifndef UVPP_PIPE_HPP
#define UVPP_PIPE_HPP

#include "uvpp_stream.hpp"

namespace uvpp
{
    class Pipe;

    struct PipeImpl
    {
        uv_pipe_t handle;
        DataReadCallback callback;
        MemPool<65536> mempool;

        ConnectionCallback<Pipe> connection_cb;
        StatusCallback connect_cb;
    };

    class Pipe : public Stream<PipeImpl>
    {
    public:
        Pipe(uv_loop_t *uvloop) {
            uv_pipe_init(uvloop, phandle(), 0);
        }

        int open(uv_file fd) {
            int rc = uv_pipe_open(phandle(), fd);
            assert(rc==0 || print_error(rc));
            return rc;
        }

        int bind(const std::string& name) {
            int rc = uv_pipe_bind(phandle(), name.c_str());
            assert(rc==0 || print_error(rc));
            return rc;
        }

        int listen(ConnectionCallback<Pipe> connection_cb) {
            pimpl->connection_cb = connection_cb;
            int rc = uv_listen(as_stream(), 0, [](uv_stream_t *server, int status){
                if (status < 0) {
                    print_error(status);
                    return;
                }
                auto pimpl = static_cast<Impl*>(server->data);
                Pipe client(server->loop);
                int rc = uv_accept(server, client.as_stream());
                // documentation guarantees success...
                assert(rc==0 || print_error(rc));
                pimpl->connection_cb(std::move(client));
            });
            assert(rc==0 || print_error(rc));
            return rc;
        }

        void connect(const std::string& name, StatusCallback connect_cb) {
            pimpl->connect_cb = connect_cb;
            auto req = new uv_connect_t();
            req->data = pimpl;
            uv_pipe_connect(req, phandle(), name.c_str(),
                [](uv_connect_t *req, int status){
                    auto pimpl = static_cast<Impl*>(req->data);
                    delete req;
                    pimpl->connect_cb(status);
            });
        }

        std::string getsockname() {
            char buffer[256];
            size_t size = sizeof(buffer);
            int rc = uv_pipe_getsockname(phandle(), buffer, &size);
            assert(rc==0 || print_error(rc));
            return std::string(buffer, size);
        }

        std::string getpeername() {
            char buffer[256];
            size_t size = sizeof(buffer);
            int rc = uv_pipe_getpeername(phandle(), buffer, &size);
            assert(rc==0 || print_error(rc));
            return std::string(buffer, size);
        }
    };
}

#endif
