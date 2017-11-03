#ifndef UVPP_HPP
#define UVPP_HPP

#include <iostream>
#include <functional>
#include <memory>

#include <stdint.h>
#include <assert.h>

#include <uv.h>

namespace uvpp
{
  typedef std::function<void()> BasicCallback;
  typedef std::function<void(int, int)> StatusEventsCallback;

  inline int print_error(int err) {
    std::cerr << uv_err_name(err) << " : " << uv_strerror(err) << std::endl;
    return 0;
  }

  class Loop
  {
  public:
    Loop()
    {
      m_uvloop.reset(new uv_loop_t());
      uv_loop_init(m_uvloop.get());
    }

    ~Loop()
    {
      if (alive()) {
        // this is to call the close callbacks of the watchers
        // so that their handle memory can get freed
        run();
      }
      int rc = uv_loop_close(m_uvloop.get());
      assert(rc==0 || print_error(rc));
    }

    operator uv_loop_t*() { return m_uvloop.get(); }

    int run(uv_run_mode mode = UV_RUN_DEFAULT) {
      return uv_run(m_uvloop.get(), mode);
    }

    bool alive() { return uv_loop_alive(m_uvloop.get()); }
    void stop() { uv_stop(m_uvloop.get()); }
    uint64_t now() { return uv_now(m_uvloop.get()); }
    void update_time() { uv_update_time(m_uvloop.get()); }

  private:
    std::unique_ptr<uv_loop_t> m_uvloop;
  };

  template <typename TypeImpl>
  class BaseHandle
  {
  public:
    bool is_active() { return uv_is_active(as_base_handle())!=0; }
    bool is_closing() { return uv_is_closing(as_base_handle())!=0; }
    void ref() { uv_ref(as_base_handle()); }
    void unref() { uv_unref(as_base_handle()); }
    bool has_ref() { return uv_has_ref(as_base_handle())!=0; }

  protected:
    typedef TypeImpl Impl;

    BaseHandle() {
        pimpl = new Impl();
		pimpl->handle.type = UV_UNKNOWN_HANDLE;
        pimpl->handle.data = pimpl;
    }

    ~BaseHandle() {
        uv_close(as_base_handle(), [](uv_handle_t *handle){
            delete static_cast<Impl*>(handle->data);
        });
    }

  public:
    BaseHandle(const BaseHandle&) = delete;
    BaseHandle& operator=(const BaseHandle&) = delete;

  protected:
    decltype(Impl::handle) *phandle() { return &pimpl->handle; }
    uv_handle_t *as_base_handle() { return reinterpret_cast<uv_handle_t*>(phandle()); }

  protected:
    Impl *pimpl;
  };

  struct SignalImpl
  {
    uv_signal_t handle;
    BasicCallback callback;
  };
  
  class Signal : public BaseHandle<SignalImpl>
  {
  public:
    Signal(Loop& uvloop) {
        uv_signal_init(uvloop, phandle());
    }

    void set_callback(BasicCallback cb) {
        pimpl->callback = cb;
    }

    void start(int signum) {
      uv_signal_start(phandle(), [](uv_signal_t *handle, int signum){
        static_cast<Impl*>(handle->data)->callback();
      }, signum);
    }

    void stop() { uv_signal_stop(phandle()); }
  };

  struct TimerImpl
  {
    uv_timer_t handle;
    BasicCallback callback;
  };

  class Timer : public BaseHandle<TimerImpl>
  {
  public:
    Timer(Loop& uvloop) {
        uv_timer_init(uvloop, phandle());
    }

    void set_callback(BasicCallback cb) {
        pimpl->callback = cb;
    }

    void start(uint64_t timeout, uint64_t repeat) {
      uv_timer_start(phandle(), [](uv_timer_t *handle){
        static_cast<Impl*>(handle->data)->callback();
      }, timeout, repeat);
    }

    void stop() { uv_timer_stop(phandle()); }
    void again() { uv_timer_again(phandle()); }
  };

#if 0
#define DEFINE_WATCHER(Klass, klass) \
  class Klass : public BaseHandle<uv_##klass##_t> {                 \
  public:                                                           \
    Klass(Loop& uvloop) { uv_##klass##_init(uvloop, m_handle); }    \
    void set_callback(BasicCallback cb) { m_callback = cb; }        \
    void start() {                                                  \
      uv_##klass##_start(m_handle, [](uv_##klass##_t *handle){      \
        static_cast<Klass*>(handle->data)->m_callback();            \
      });                                                           \
    }                                                               \
    void stop() { uv_##klass##_stop(m_handle); }                    \
  private:                                                          \
    BasicCallback m_callback;                                       \
  };

  DEFINE_WATCHER(Prepare, prepare)
  DEFINE_WATCHER(Check, check)
  DEFINE_WATCHER(Idle, idle)

#undef DEFINE_WATCHER

  class Poll : public BaseHandle<uv_poll_t>
  {
  public:
    Poll(Loop& uvloop) : m_uvloop(uvloop) {
    }

    Poll(Loop& uvloop, uv_os_sock_t sock) : Poll(uvloop) {
      init(sock);
    }

    void init(uv_os_sock_t sock) {
      assert(m_handle->type==UV_UNKNOWN_HANDLE);
      uv_poll_init_socket(m_uvloop, m_handle, sock);
    }

    void set_callback(StatusEventsCallback cb) {
      m_callback = cb;
    }

    void start(int events) {
      assert(m_handle->type==UV_POLL);
      uv_poll_start(m_handle, events, [](uv_poll_t *handle, int status, int events){
        static_cast<Poll*>(handle->data)->m_callback(status, events);
      });
    }

    void stop() { uv_poll_stop(m_handle); }

  private:
    uv_loop_t *m_uvloop;
    StatusEventsCallback m_callback;
  };
#endif
} // namespace uvpp
#endif

