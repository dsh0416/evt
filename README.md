# Evt

The Event Library that designed for Ruby 3.0.

**This gem is still under development, APIs and features are not stable. Advices and PRs are highly welcome.**

[![Build Status](https://dev.azure.com/dsh0416/evt/_apis/build/status/dsh0416.evt?branchName=master)](https://dev.azure.com/dsh0416/evt/_build/latest?definitionId=2&branchName=master)

## Features



### IO Backend Support

|                 | Linux                                | Windows                     | macOS                                                        | FreeBSD    |
| --------------- | ------------------------------------ | --------------------------- | ------------------------------------------------------------ | ---------- |
| io_uring        | ✅ <br />when liburing is installed   | ❌                           | ❌                                                            | ❌          |
| epoll           | ✅ <br />when kernel version >= 2.6.8 | ❌                           | ❌                                                            | ❌          |
| kqueue          | ❌                                    | ❌                           | ⚠️ <br />`kqueue` performance in Darwin is very poor.<br />**MAY BE DISABLED IN THE FUTURE.** | ✅          |
| IOCP            | ❌                                    | ⚠️ <br />Working in Progress | ❌                                                            | ❌          |
| Ruby (`select`) | ✅ Fallback                           | ✅ Fallback                  | ✅ Fallback                                                   | ✅ Fallback |

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
