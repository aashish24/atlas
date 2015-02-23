#ifndef atlas_LogFormat_h
#define atlas_LogFormat_h

#include <string>
#include <stack>
#include "eckit/log/FormatBuffer.h"
#include "eckit/log/FormatChannel.h"

namespace atlas {

class LogFormat : public eckit::FormatBuffer {
public:

    LogFormat( std::size_t size = 1024 );

    virtual ~LogFormat(){ pubsync(); }

    const std::string& prefix() const;

    void set_prefix( const std::string& );

    void indent( const std::string& = std::string("  ") );

    void dedent();

    void clear_indentation();

private:

    std::string parsed_prefix() const;
    virtual void beginLine();
    virtual void endLine();

private:

    std::vector<std::string> indent_stack_;
    std::string indent_;
    std::string prefix_;

    mutable std::map<std::string,std::string> subst_;

};

class FormattedChannel : public eckit::FormatChannel {
public:

  FormattedChannel( std::ostream* channel, LogFormat* format );

  FormattedChannel( std::ostream& channel, LogFormat* format );

  virtual ~FormattedChannel();

  const LogFormat& format() const { return *format_; };
        LogFormat& format()       { return *format_; };

private:
  std::ostream* channel_;
  LogFormat* format_;
};


} // namespace atlas

#endif // atlas_LogFormat_h
