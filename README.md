# Evt

The Event Library that designed for Ruby 3.0.

**This gem is still under development, APIs and features are not stable. Advices and PRs are highly welcome.**

![CI Tests](https://github.com/dsh0416/evt/workflows/CI%Tests/badge.svg)

## Features



### IO Backend Support

|                 | Linux                                | Windows                     | macOS         | FreeBSD    |
| --------------- | ------------------------------------ | --------------------------- | ------------- | ---------- |
| io_uring        | ✅ <br />when liburing is installed   | ❌                           | ❌             | ❌          |
| epoll           | ✅ <br />when kernel version >= 2.6.8 | ❌                           | ❌             | ❌          |
| kqueue          | ❌                                    | ❌                           | ⚠️ (Check <1>) | ✅          |
| IOCP            | ❌                                    | ⚠️ <br />Working in Progress | ❌             | ❌          |
| Ruby (`select`) | ✅ Fallback                           | ✅ Fallback                  | ✅ Fallback    | ✅ Fallback |

<1>:  `kqueue` performance in Darwin is very poor. **MAY BE DISABLED IN THE FUTURE.**

## Install

```bash
gem install evt
```

## Usage

```ruby
require 'evt'

rd, wr = IO.pipe
Thread.current.scheduler = Evt::Scheduler.new

hit = 0
fiber = Fiber.new do
    scheduler.wait_readable(rd)
    hit += 1
end

wr.write('Hello World')
fiber.resume
Thread.current.scheduler.run

puts hit # => 1
```
