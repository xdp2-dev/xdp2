#ifndef PROTO_XDP2GEN_LOG_HANDLER_H
#define PROTO_XDP2GEN_LOG_HANDLER_H

#include <ostream>

namespace xdp2gen
{

class log_handler {
private:
    using ostream_ref = std::reference_wrapper<std::ostream>;

    log_handler(std::ostream &os, bool enabled)
        : _os(os)
        , _enabled(enabled){};

    std::ostream &_os;

    static bool _display_warning;
    static bool _display_log;
    bool _enabled;

public:
    log_handler() = delete;
    log_handler(log_handler &log_handler_other) = delete;
    void operator=(log_handler const &log_handler_other) = delete;
    void operator=(log_handler const &&log_handler_other) = delete;

    static log_handler log(std::ostream &os)
    {
        return { os, log_handler::_display_log };
    }

    static log_handler warning(std::ostream &os)
    {
        return { os, log_handler::_display_warning };
    }

    static void enable_log()
    {
        log_handler::_display_log = true;
    }

    static void disable_log()
    {
        log_handler::_display_log = false;
    }

    static void switch_log_mode()
    {
        log_handler::_display_log = !log_handler::_display_log;
    }

    static bool is_display_log()
    {
        return log_handler::_display_log;
    }

    static void set_display_log(bool display_log)
    {
        log_handler::_display_log = display_log;
    }

    static void enable_warning()
    {
        log_handler::_display_warning = true;
    }

    static void disable_warning()
    {
        log_handler::_display_warning = false;
    }

    static void switch_warning_mode()
    {
        log_handler::_display_warning = !log_handler::_display_warning;
    }

    static bool is_display_warning()
    {
        return log_handler::_display_warning;
    }

    static void set_display_warning(bool display_warning)
    {
        log_handler::_display_warning = display_warning;
    }

    template <typename T>
    log_handler &operator<<(T &rhs)
    {
        if (this->_enabled)
            _os << rhs;

        return *this;
    }

    template <typename T>
    log_handler &operator<<(T &&rhs)
    {
        if (this->_enabled)
            _os << rhs;

        return *this;
    }

    log_handler &operator<<(std::ostream &(*f)(std::ostream &))
    {
        if (this->_enabled)
            _os << f;

        return *this;
    }
};

bool log_handler::_display_warning = true;
bool log_handler::_display_log = false;

}

using plog = xdp2gen::log_handler;

#endif
