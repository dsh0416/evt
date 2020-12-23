# Evt

The Event Library that designed for Ruby 3.0 Fiber Scheduler.

**This gem is still under development, APIs and features are not stable. Advices and PRs are highly welcome.**

[![CI Tests](https://github.com/dsh0416/evt/workflows/CI%20Tests/badge.svg)](https://github.com/dsh0416/evt/actions?query=workflow%3A%22Build%22)
[![Build](https://github.com/dsh0416/evt/workflows/Build/badge.svg)](https://github.com/dsh0416/evt/actions?query=workflow%3A%22CI+Tests%22)
[![Gem Version](https://badge.fury.io/rb/evt.svg)](https://rubygems.org/gems/evt)
[![Downloads](https://ruby-gem-downloads-badge.herokuapp.com/evt?type=total)](https://rubygems.org/gems/evt)

## Features

### IO Backend Support

|                 | Linux       | Windows     | macOS       | FreeBSD     |
| --------------- | ----------- | ------------| ----------- | ----------- |
| io_uring        | ⚠️  (See 1) | ❌          | ❌          | ❌          |
| epoll           | ✅  (See 2) | ❌          | ❌          | ❌          |
| kqueue          | ❌          | ❌          | ✅ (⚠️ See 5) | ✅          |
| IOCP            | ❌          | ❌ (⚠️See 3) | ❌          | ❌          |
| Ruby (`IO.select`) | ✅ (See 6) | ✅ (⚠️See 4) | ✅ (See 6) | ✅ (See 6) |

1. when liburing is installed. (Currently fixing)
2. when kernel version >= 2.6.9
3. WOULD NOT WORK until `FILE_FLAG_OVERLAPPED` is included in I/O initialization process.
4. Some I/Os are not able to be nonblock under Windows. Using POSIX select, **SLOW**. See [Scheduler Docs](https://docs.ruby-lang.org/en/master/doc/scheduler_md.html#label-IO).
5. `kqueue` performance in Darwin is very poor. **MAY BE DISABLED IN THE FUTURE.**
6. Using poll

### Benchmark

The benchmark is running under `v0.3.6` version. See `example.rb` in [midori](https://github.com/midori-rb/midori.rb) for test code, the test is running under a single-thread server.

The test command is `wrk -t4 -c8192 -d30s http://localhost:8080`.

All of the systems have set their file descriptor limit to maximum.
On systems raising "Fiber unable to allocate memory", `sudo sysctl -w vm.max_map_count=1000000` is set.

| OS    | CPU         | Memory | Backend                | req/s         |
| ----- | ----------- | ------ | ---------------------- | ------------- |
| Linux | Ryzen 2700x | 64GB   | epoll                  | 2035742.59    |
| Linux | Ryzen 2700x | 64GB   | io_uring               | require fixes |
| Linux | Ryzen 2700x | 64GB   | IO.select (using poll) | 1837640.54    |
| macOS | i7-6820HQ   | 16GB   | kqueue                 | 257821.78     |
| macOS | i7-6820HQ   | 16GB   | IO.select (using poll) | 338392.12     |

## Install

```bash
gem install evt
```

## Usage

```ruby
require 'evt'

rd, wr = IO.pipe
scheduler = Evt::Scheduler.new

Fiber.set_scheduler scheduler

Fiber.schedule do
  message = rd.read(20)
  puts message
  rd.close
end

Fiber.schedule do
  wr.write("Hello World")
  wr.close
end

# "Hello World"
```

## Roadmap

- [x] Support epoll/kqueue/select
- [x] Upgrade to the latest Scheduler API
- [x] Support io_uring
- [x] Support iov features of io_uring
- [x] Support IOCP (**NOT ENABLED YET**)
- [x] Setup tests with Ruby 3
- [x] Selectable backend compilation by environment variable
- [ ] Support IOCP with iov features
- [ ] Setup more tests for production purpose
- [ ] Documentation for usages