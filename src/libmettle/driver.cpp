#include <iostream>
#include <vector>

#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/program_options.hpp>

#include <mettle/driver/run_tests.hpp>
#include <mettle/driver/cmd_line.hpp>
#include <mettle/driver/log/child.hpp>
#include <mettle/driver/log/summary.hpp>
#include <mettle/driver/log/term.hpp>
#include <mettle/suite/compiled_suite.hpp>

#include "forked_test_runner.hpp"

namespace mettle {

namespace {
  struct all_options : generic_options, output_options, child_options {
    METTLE_OPTIONAL_NS::optional<int> child_fd;
  };

  void report_error(const std::string &program_name,
                    const std::string &message) {
    std::cerr << program_name << ": " << message << std::endl;
  }
}

namespace detail {

  int drive_tests(int argc, const char *argv[], const suites_list &suites) {
    using namespace mettle;
    namespace opts = boost::program_options;

    auto factory = make_logger_factory();

    all_options args;
    auto generic = make_generic_options(args);
    auto output = make_output_options(args, factory);
    auto child = make_child_options(args);

    opts::options_description hidden("Hidden options");
    hidden.add_options()
      ("child", opts::value(&args.child_fd), "run this file as a child process")
    ;

    opts::variables_map vm;
    try {
      opts::options_description all;
      opts::positional_options_description pos;
      all.add(generic).add(output).add(child).add(hidden);
      auto parsed = opts::command_line_parser(argc, argv)
        .options(all).positional(pos).run();

      opts::store(parsed, vm);
      opts::notify(vm);
    } catch(const std::exception &e) {
      report_error(argv[0], e.what());
      return 2;
    }

    if(args.show_help) {
      opts::options_description displayed;
      displayed.add(generic).add(output).add(child);
      std::cout << displayed << std::endl;
      return 0;
    }

    test_runner runner;
    if(args.no_fork) {
      if(args.timeout) {
        report_error(argv[0], "--timeout requires forking tests");
        return 2;
      }
      runner = inline_test_runner;
    }
    else {
      runner = forked_test_runner(args.timeout);
    }

    if(args.child_fd) {
      if(auto output_opt = has_option(output, vm)) {
        using namespace opts::command_line_style;
        report_error(argv[0], output_opt->canonical_display_name(allow_long) +
                              " can't be used with --child");
        return 2;
      }

      namespace io = boost::iostreams;
      io::stream<io::file_descriptor_sink> fds(
        *args.child_fd, io::never_close_handle
      );
      log::child logger(fds);
      run_tests(suites, logger, runner, args.filters);
      return 0;
    }

    if(args.no_fork && args.show_terminal) {
      report_error(argv[0], "--show-terminal requires forking tests");
      return 2;
    }

    if(args.runs == 0) {
      report_error(argv[0], "no test runs, exiting");
      return 1;
    }

    try {
      term::enable(std::cout, args.color);
      indenting_ostream out(std::cout);

      log::summary logger(
        out, factory.make(args.output, out, args), args.show_time,
        args.show_terminal
      );
      for(size_t i = 0; i != args.runs; i++)
        run_tests(suites, logger, runner, args.filters);

      logger.summarize();
      return !logger.good();
    }
    catch(const std::exception &e) {
      report_error(argv[0], e.what());
      return 3;
    }
  }
}

} // namespace mettle
