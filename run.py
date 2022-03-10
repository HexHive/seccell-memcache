#!/usr/bin/env python3
import argparse
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

def run():
  for runs in [20]:
    for size in [32*1024, 64*1024, 96*1024]:
      pin_path = os.getcwd() + '/memtrace/pin/pin'
      pintool_path = os.getcwd() + '/memtrace/pintool/obj-intel64/memtrace.so'
      workload = os.getcwd() + '/src/main'
      workload_args = [str(runs), str(size)]
      data_filename = os.getcwd() + '/pin_{}_{}.log'.format(runs, readable(size))
      pin_cmd = [pin_path, '-t', pintool_path, '-d', data_filename, '--', workload] + workload_args
      # Run workload with PIN
      sp.run(pin_cmd)
      
      print('runs: {}, size: {}'.format(runs, readable(size)), end='')
      
      tlbcache = os.getcwd() + '/tracing/main'
      tlb_cmd = [tlbcache, data_filename]
      # Trace workload
      sp.run(tlb_cmd, capture_output=True)
  
  
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


