#!/usr/bin/env python
# -*- coding: utf-8 -*-

# 循环读取文件中一行的数据，计算其中单词出现的次数
def count(filename, word):
    cnt = 0
    with open(filename, 'r') as f:
        for line in f:
            if line.strip() == word:
                cnt += 1
    return cnt

if __name__ == '__main__':
    print(count('results/memcheck_k32.log', 'new DeltaPage'))
    print(count('results/memcheck_k32.log', 'delete DeltaPage'))