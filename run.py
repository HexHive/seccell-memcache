#!/usr/bin/env python3
import argparse
import datetime
import os
import subprocess as sp

def readable(size):
  if size < 1024:
    return str(size)
  elif size < 1024 * 1024:
    return str(int(size/1024)) + 'K'
  elif size < 1024 * 1024 * 1024:
    return str(int(size/(1024 * 1024))) + 'M'
  else:
    return str(int(size/(1024 * 1024 * 1024))) + 'G'
  
def run_datapoint(data_dir, runs, size): 
  pin_path = os.getcwd() + '/memtrace/pin/pin'
  pintool_path = os.getcwd() + '/memtrace/pintool/obj-intel64/memtrace.so'
  workload = os.getcwd() + '/src/main'
  workload_args = [str(runs), str(size)]
  data_filename = data_dir + '/pin_{}_{}.log'.format(runs, readable(size))
  pin_cmd = [pin_path, '-t', pintool_path, '-d', data_filename, '--', workload] + workload_args
  # Run workload with PIN
  sp.run(pin_cmd)
  
  tlbcache = os.getcwd() + '/tracing/main'
  tlb_cmd = [tlbcache, data_filename]
  # Trace workload
  traceproc = sp.run(tlb_cmd, capture_output=True)
  
  print('runs: {}, size: {}'.format(runs, readable(size)))
  print(traceproc.stdout.decode('utf-8'))
  with open(data_dir + '/log', 'a') as logfd:
    print('runs: {}, size: {}'.format(runs, readable(size)), end='', file = logfd)
    print(traceproc.stdout, file = logfd)

def run():
  dt = datetime.datetime.now()
  data_dir = os.getcwd() + '/data_{:02n}_{:02n}_{:02n}_{:02n}_{:02n}_{:02n}'.format(dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second)
  os.mkdir(data_dir)
  
  with open(data_dir + '/log', 'x') as logfd:
    pass
  for runs in [3]:
    for size in [1*1024, 4 * 1024, 16 * 1024, 32*1024, 64*1024, 96*1024, 128*1024, 192*1024, 256*1024, 384*1024, 512*1024]:     
      run_datapoint(data_dir, runs, size)
  
def build(clean = False):
  mycache_build_cmd = ['make', '-C', 'src']
  mycache_clean_cmd = ['make', '-C', 'src', 'clean']
  pintool_build_cmd = ['make', '-C', 'memtrace/pintool']
  pintool_clean_cmd = ['make', '-C', 'memtrace/pintool', 'clean']
  tlbcache_build_cmd = ['make', '-C', 'tracing']
  tlbcache_clean_cmd = ['make', '-C', 'tracing', 'clean']

  # Build mycache
  if(clean):
    sp.run(mycache_clean_cmd)
  sp.run(mycache_build_cmd)
  # Build memtrace pintool
  if(clean):
    sp.run(pintool_clean_cmd)
  sp.run(pintool_build_cmd)
  # Build tlb-cache model
  if(clean):
    sp.run(tlbcache_clean_cmd)
  sp.run(tlbcache_build_cmd)

def main():
  parser = argparse.ArgumentParser(
          description='Run script for experiment')
  parser.add_argument('-c', '--clean', action='store_true', default=False,
                      help = 'Clean directories before building')
  parser.add_argument('-b', '--build', action='store_true', default=False,
                      help = 'Rebuild programs')
  args = parser.parse_args()
  
  if(args.build):
    build(args.clean)
  run()
  
if __name__ == '__main__':
  main()


