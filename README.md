# evt

[![Build Status](https://travis-ci.org/dsh0416/evt.svg?branch=master)](https://travis-ci.org/dsh0416/evt)

A Handcrafted Low-Level Event Handler designed as Ruby 3 Scheduler. 

Supports `epoll`, `kqueue`, IOCP (WIP), and Ruby `select` fallback.

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
