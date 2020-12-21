# Evt

The Event Library that designed for Ruby 3.0.

**This gem is still under development, APIs and features are not stable. Advices and PRs are highly welcome.**

[![CI Tests](https://github.com/dsh0416/evt/workflows/CI%20Tests/badge.svg)](https://github.com/dsh0416/evt/actions?query=workflow%3A%22CI+Tests%22)
[![Gem Version](https://badge.fury.io/rb/evt.svg)](https://rubygems.org/gems/evt)
[![Downloads](https://ruby-gem-downloads-badge.herokuapp.com/evt?type=total)](https://rubygems.org/gems/evt)

## Features

### IO Backend Support

|                 | Linux       | Windows     | macOS       | FreeBSD     |
| --------------- | ----------- | ------------| ----------- | ----------- |
| io_uring        | ✅  (See 1) | ❌          | ❌          | ❌          |
| epoll           | ✅  (See 2) | ❌          | ❌          | ❌          |
| kqueue          | ❌          | ❌          | ✅ (⚠️ See 5) | ✅          |
| IOCP            | ❌          | ❌ (⚠️See 3) | ❌          | ❌          |
| Ruby (`select`) | ✅ Fallback | ✅ (⚠️See 4) | ✅ Fallback | ✅ Fallback |

1. when liburing is installed
2. when kernel version >= 2.6.8
3. WOULD NOT WORK until `FILE_FLAG_OVERLAPPED` is included in I/O initialization process.
4. Some I/Os are not able to be nonblock under Windows. See [Scheduler Docs](https://docs.ruby-lang.org/en/master/doc/scheduler_md.html#label-IO).
5. `kqueue` performance in Darwin is very poor. **MAY BE DISABLED IN THE FUTURE.**

### Benchmark

The benchmark is running under `v0.2.1` version. See [evt-server-benchmark](https://github.com/dsh0416/evt-server-benchmark) for test code, the test is running under a single-thread server.

The test command is `wrk -t4 -c1000 -d30s http://localhost:3001`.

All of the systems have set their file descriptor limit to maximum.

| OS    | CPU         | Memory | Backend                | req/s    |
| ----- | ----------- | ------ | ---------------------- | -------- |
| Linux | Ryzen 2700x | 64GB   | epoll                  | 41492.13 |
| Linux | Ryzen 2700x | 64GB   | io_uring               | 45309.40 |
| Linux | Ryzen 2700x | 64GB   | IO.select (using poll) | 6621.82  |
| Linux | Ryzen 2700x | 64GB   | Blocking I/O           | 1732.34  |
| macOS | i7-6820HQ   | 16GB   | kqueue                 | 1271.79  |
| macOS | i7-6820HQ   | 16GB   | IO.select (using poll) | 1572.90  |

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

scheduler.run

# "Hello World"
```

## Roadmap

- [x] Support epoll/kqueue/select
- [x] Upgrade to the latest Scheduler API
- [x] Support io_uring
- [x] Support iov features of io_uring
- [x] Support IOCP (**NOT ENABLED YET**)
- [x] Setup tests with Ruby 3
- [ ] Selectable backend compilation by environment variable
- [ ] Support IOCP with iov features
- [ ] Setup more tests for production purpose
- [ ] Documentation for usages