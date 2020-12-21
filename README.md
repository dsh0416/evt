# evt

[![Build Status](https://dev.azure.com/dsh0416/evt/_apis/build/status/dsh0416.evt?branchName=master)](https://dev.azure.com/dsh0416/evt/_build/latest?definitionId=2&branchName=master)

A Handcrafted Low-Level Event Handler designed as Ruby 3 Scheduler. 

Supports `io_uring`, `epoll`, `kqueue`, IOCP (WIP), and Ruby `select` fallback.

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
