#include <cstddef>
#include <iostream>
#include <stdexcept>
#include "LogData.hh"
#include <cstring>
#include <thread>
#include <fstream>
#include <cstdlib>
#include <unistd.h>
#include <getopt.h>
#include <atomic>
#include <vector>

static LogData *logs;

static const std::size_t MAX_BUF = 65535;

static unsigned int threads = 4;
static unsigned int field = 0;
static unsigned int data_field = 0;	// field to match
static char match[10]; // string to match. ex: false, true, 200, 500
static uint64_t step = 5000;
static bool relative = false;
static int count_flag = 0;	// flag to count number of occurences (default)
static int avg_flag = 0;	// calculate average and std deviance
static int stddev_flag = 1;	// calculate variance and standard deviation
static std::atomic<uint64_t> total_samples(0);
static std::atomic<uint64_t> total_data(0);

/*
	https://twiki.cern.ch/twiki/bin/view/CMSPublic/FWMultithreadedThreadSafeDataStructures
	http://baptiste-wicht.com/posts/2012/07/c11-concurrency-tutorial-part-4-atomic-type.html
	http://stackoverflow.com/questions/16919858/thread-safe-lock-free-array

	Usage:
		AtomicUInt64 value;
		myVector->push_back ( value );

		or:

		AtomicUInt64 value(x);
		myVector->push_back ( value );
*/
template<typename T>
struct MobileAtomic
{
  std::atomic<T> atomic;

  MobileAtomic() : atomic(T()) {}

  explicit MobileAtomic ( T const& v ) : atomic ( v ) {}
  explicit MobileAtomic ( std::atomic<T> const& a ) : atomic ( a.load() ) {}

  MobileAtomic ( MobileAtomic const&other ) : atomic( other.atomic.load() ) {}

  MobileAtomic& operator=( MobileAtomic const &other )
  {
    atomic.store( other.atomic.load() );
    return *this;
  }
};

typedef MobileAtomic<uint64_t> AtomicUInt64;

// static uint64_t first = 0;
// static uint64_t last = 0;
// static bool has_first = false;
// static bool has_last = false;

static void parse_cmd_line(int argc, char *argv[]);
//static bool parse_interval(const char *);
static void usage(const char *);
static void do_main();
static void thread_main(std::size_t, std::size_t);
static void nonseekable_stream();
static void multi_thread();
static bool readline(char *buf, std::size_t n, std::istream& f);
static void parse_line(const char *buf, std::size_t n);
static void print_result();


int main(int argc, char *argv[]) {
	parse_cmd_line(argc, argv);
	std::cin.exceptions(std::istream::badbit);
	try {
		do_main();
	} catch (const std::exception& e) {
		std::cerr << "Exception: " << e.what() << '.' << std::endl;
		print_result();
		return 1;
	} catch (...) {
		std::cerr << "Exception." << std::endl;
		print_result();
		return 1;
	}
	print_result();
	return 0;
}

static void
parse_cmd_line(int argc, char *argv[]) {
	char c;
	static struct option long_options[] = {
            /* These options set a flag. */
            {"count",   no_argument,        &count_flag,    1},
            {"avg",     no_argument,        &avg_flag,      0},
            {"stddev",  no_argument,        &stddev_flag,   0},
            /* These options don’t set a flag.
             We distinguish them by their indices. */
            {"help",    no_argument,        0,              'h'},
            {"all",     no_argument,        0,              'a'},
            {"thread",  required_argument,  0,              't'},
            {"dataidx", required_argument,  0,              'd'},
            {"match",   required_argument,  0,              'm'},
            {"field",   required_argument,  0,              'f'},
            {"step",    required_argument,  0,              's'},
            {0,         0,                  0,                  0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;
	while ((c = getopt_long(argc, argv, "hat:d:m:f:s:i:", long_options, &option_index)) != EOF)
		switch (c) {
                case 0:
                        /* If this option set a flag, do nothing else now. */
                        if (long_options[option_index].flag != 0)
                          break;
                        break;
		case 'h':
			usage(argv[0]);
			exit(0);
		case 'a':
			relative = false;
			break;
		case 't': {
			if (nullptr == optarg) {
				std::cerr << "-t must be followed by the number of threads" << std::endl;
				usage(argv[0]);
				exit(1);
			}
			errno = 0;
			char *endp;
			long n = strtol(optarg, &endp, 10);
			if (0 != errno || '\0' != *endp || n <= 0 || n > 1024) {
				std::cerr << "The number of threads must belong to the interval {1, 2, ..., 1024}" << std::endl;
				usage(argv[0]);
				exit(1);
			}
			threads = n;
			break;
		}
		case 'f': {
			if (nullptr == optarg) {
				std::cerr << "-f must be followed by the zero-based field index" << std::endl;
				usage(argv[0]);
				exit(1);
			}
			errno = 0;
			char *endp;
			long n = strtol(optarg, &endp, 10);
			if (0 != errno || '\0' != *endp || n < 0) {
				std::cerr << "The field index must be a positive integer" << std::endl;
				usage(argv[0]);
				exit(1);
			}
			field = n;
			break;
		}
		case 'd': {
                        if (nullptr == optarg) {
                            std::cerr << "-d must be followed by the data field index" << std::endl;
                            usage(argv[0]);
                            exit(1);
                        }
                        errno = 0;
                        char *endp;
                        long n = strtol(optarg, &endp, 10);
                        if (0 != errno || '\0' != *endp || n < 0) {
                            std::cerr << "The field index must be a positive integer" << std::endl;
                            usage(argv[0]);
                            exit(1);
                        }
                        data_field = n;
                        break;
                    }
                    case 'm': {
                        if (nullptr == optarg) {
                            std::cerr << "-m must be followed by the string to match" << std::endl;
                            usage(argv[0]);
                            exit(1);
                        }
                        errno = 0;
                                    if(strlen(optarg) > 10) {
                                            std::cerr << "-m argument must have less than 9 characters" << std::endl;
                                            usage(argv[0]);
                                            exit(1);
                                    }
                                    strcpy(optarg,match);
                        break;
                    }
		case 's': {
			if (nullptr == optarg) {
				std::cerr << "-s must be followed by the step of the histogram" << std::endl;
				usage(argv[0]);
				exit(1);
			}
			errno = 0;
			char *endp;
			long n = strtol(optarg, &endp, 10);
			if (0 != errno || '\0' != *endp || n <= 0) {
				std::cerr << "The histogram step must be a strictly positive integer" << std::endl;
				usage(argv[0]);
				exit(1);
			}
			step = n;
			break;
		}
		case '?':
		default:
			usage(argv[0]);
			exit(1);
		}
}

// static bool
// parse_interval(const char *s) {
// 	char *endp;
// 	if (':' != *s) {
// 		errno = 0;
// 		long long b = strtoll(s, &endp, 10);
// 		if (0 != errno || ('\0' != *endp && ':' != *endp) || b < 0)
// 			return false;
// 		has_first = true;
// 		first = b;
// 		if ('\0' == *endp)
// 			return true;
// 		s = endp + 1;
// 	} else
// 		++s;
// 	if ('\0' == *s)
// 		return true;
// 	errno = 0;
// 	long long e = strtoll(s, &endp, 10);
// 	if (0 != errno || '\0' != *endp || e < 0 || (has_first && first > e))
// 		return false;
// 	has_last = true;
// 	last = e;
// 	return false;
// }

static void
usage(const char *s) {
	std::cerr << "Usage:" << std::endl
		  << s << "[-t threads] [-f field] [-i start:end] [-s step]"
		  << std::endl;
}

static void
do_main() {
	std::cin.seekg(0, std::ios_base::end);
	if (!std::cin.fail())
		multi_thread();
	else {
		std::cin.clear();
		nonseekable_stream();
	}
}

static void
multi_thread() {
	static char buf[MAX_BUF + 1];
	logs = new SyncLogData;
	std::size_t sz = std::cin.tellg();
	if (std::cin.fail())
		throw std::runtime_error("tellg.");

	std::size_t thread_chunk = sz / threads;
	std::size_t ndx[threads + 1];
	ndx[0] = 0;

	unsigned int i = 1;
	for (; i < threads; ++i) {
		std::cin.seekg(i * thread_chunk, std::ios_base::beg);
		if (std::cin.fail())
			throw std::runtime_error("seekg.");
		if (!readline(buf, sizeof(buf), std::cin))
			break;
		ndx[i] = i * thread_chunk + std::cin.gcount();
	}
	ndx[i] = sz;

	std::thread *t[i];

	for (unsigned int j = 0; j < i; ++j)
		t[j] = new std::thread(&thread_main, ndx[j], ndx[j + 1] - ndx[j]);

	for (unsigned int j = 0; j < i; ++j) {
		t[j]->join();
		delete t[j];
	}
	
}

static void
thread_main(std::size_t s, std::size_t n) {
	char buf[MAX_BUF + 1];
	std::ifstream is;
	is.exceptions(std::ifstream::badbit);
	is.open("/proc/self/fd/0");
	is.seekg(s, std::ios_base::beg);
	if (is.fail())
		throw std::runtime_error("seekg.");
	for (std::size_t read = 0; read < n && readline(buf, sizeof(buf), is);) {
		std::size_t line_length = is.gcount();
		read += line_length;
		parse_line(buf, line_length);
	}
}

static void
nonseekable_stream() {
	static char buf[MAX_BUF + 1];
	logs = new LogData;
	while (readline(buf, sizeof(buf), std::cin))
		parse_line(buf, std::cin.gcount());
}

static bool
readline(char *buf, std::size_t n, std::istream& f) {
	f.getline(buf, n);
	if (f.fail()) {
		if (!f.eof())
			// no sentry construction
			// line too long
			throw std::runtime_error("Line too long.");
		return false;
	}
	return true;
}

static void
parse_line(const char *buf, std::size_t n) {
	if (n > 0)
		--n;
	const char *e = buf + n;
	const char *old_comma = buf;
	const char *comma;
	unsigned int i = 0;
	do {
		comma = reinterpret_cast<const char *>(memchr(old_comma, ',', e - old_comma));
		++i;
		if (i > field)
			break;
		if (nullptr == comma)
			throw std::runtime_error("Line does not contain the required field");
		old_comma = comma + 1;
	} while (true);

	if (nullptr == comma)
		comma = e;

	errno = 0;
	char *endp;
	long long x = strtoll(old_comma, &endp, 10);
	if (0 != errno || comma != endp || x < 0)
		throw std::runtime_error("The field is not a positive integer");
	logs->add(x, 1);
	++total_samples;
}

static void
print_result() {
	LogData::const_iterator i = logs->begin();
	LogData::const_iterator e = logs->end();
	if (i == e)
		return;
	uint64_t ss = i->first;
	uint64_t s = ss;
	uint64_t N = 0;
	bool f[] = {false, false, false, false, false, false, false};
	static const double fractions[] = {0.25, 0.5, 0.75, 0.8, 0.9, 0.95, 0.99};
	do {
		uint64_t x = s + step;
		LogData::const_iterator j = logs->lower_bound(x); // >=
		std::size_t samples = 0;
		for (; i != j; ++i) {
			samples += i->second;
			N += i->second;
			for (unsigned int k = 0; k < sizeof(f) / sizeof(f[0]); ++k)
				if (!f[k] && N >= total_samples * fractions[k]) {
					f[k] = true;
					std::cerr << fractions[k] << ":\t" << (relative ? i->first - ss : s) << std::endl;
				}
		}
		std::cout << (relative ? s - ss : s)
			  << '\t'
			  << static_cast<double>(samples) / step << std::endl;
		if (j == e)
			break;
		s = x;
	} while (true);
}
