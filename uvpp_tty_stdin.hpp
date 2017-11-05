#ifndef UVPP_TTY_STDIN_HPP
#define UVPP_TTY_STDIN_HPP

#include "uvpp_stream.hpp"

namespace uvpp
{
    struct TtyStdinImpl
    {
        uv_tty_t handle;
        DataReadCallback callback;
        MemPool<65536> mempool;
    };

    class TtyStdin : public Stream<TtyStdinImpl>
    {
    public:
        TtyStdin(Loop& uvloop) {
            uv_tty_init(uvloop, phandle(), 0, 1);
        }
    };
}

#endif
