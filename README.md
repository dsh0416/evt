# evt
A low-level Event Handler designed for Ruby 3 Scheduler

```ruby
rd, wr = IO.pipe
Thread.current.scheduler = Scheduler.new

hit = 0
fiber = Fiber.new do
    scheduler.wait_readable(rd)
    hit += 1
end


wr.write('Hello World')
fiber.resume
Thread.current.scheduler.run

assert_equal hit, 1
```
