# run speed tests, return number of ticks
def run key_max, threads, iters, task, n = 1, branch = nil
  branch = [branch] unless branch.is_a? Enumerable
  key_max = [key_max] unless key_max.is_a? Enumerable
  threads = [threads] unless threads.is_a? Enumerable
  iters = [iters] unless iters.is_a? Enumerable
  task = [task] unless task.is_a? Enumerable

  data = []

  branch.each do |b|
    system "git checkout #{b} && make" if b
    key_max.each do |k|
      threads.each do |t|
        iters.each do |i|
          task.each do |a|
            y = []
            y << b unless branch.count == 1
            y << k unless key_max.count == 1
            y << t unless threads.count == 1
            y << i unless iters.count == 1
            y << a unless task.count == 1
            y << n.times.map { Integer(`./main -k #{k} -t #{t} -i #{i} -a #{a}`, 10) }.reduce(:+) / n.to_f
            y = y[0] if y.size == 1
            data << y
          end
        end
      end
    end
  end

  data = data[0] if data.size == 1

  data
end
