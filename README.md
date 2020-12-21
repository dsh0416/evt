# Evt

The Event Library that designed for Ruby 3.0.

**This gem is still under development, APIs and features are not stable. Advices and PRs are highly welcome.**

![CI Tests](https://github.com/dsh0416/evt/workflows/CI%20Tests/badge.svg)

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
3. WOULD NOT WORK until `FILE_FLAG_OVERLAPPED` is including in I/O initialization process.
4. Some I/Os are not able to be nonblock under Windows. It may raise `IO::EWOULDBLOCKWaitReadable (A non-blocking socket operation could not be completed immediately. - read would block)` when using pure nonblock I/O. But it would block in Windows.
5. `kqueue` performance in Darwin is very poor. **MAY BE DISABLED IN THE FUTURE.**

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

scheduler.eventloop

# "Hello World"
```

## Roadmap

- [x] Support epoll/kqueue/select
- [x] Upgrade to the latest Scheduler API
- [x] Support io_uring
- [x] Support iov features of io_uring
- [ ] Support IOCP ()
- [x] Setup tests with Ruby 3
- [ ] Support IOCP with iov features
- [ ] Setup more tests for production purpose
- [ ] Documentation for usages