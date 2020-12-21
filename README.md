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
| kqueue          | ❌          | ❌          | ⚠️ (See 4)  | ✅          |
| IOCP            | ❌          | ⚠️ (See 3)  | ❌          | ❌          |
| Ruby (`select`) | ✅ Fallback | ✅ Fallback | ✅ Fallback | ✅ Fallback |

1. when liburing is installed
2. when kernel version >= 2.6.8
3. Working in Progress
4. `kqueue` performance in Darwin is very poor. **MAY BE DISABLED IN THE FUTURE.**

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
