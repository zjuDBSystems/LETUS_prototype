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
    new_base_cnt = count('results/memcheck_k32.log', 'new BasePage')
    del_base_cnt = count('results/memcheck_k32.log', 'delete BasePage')
    new_delta_cnt = count('results/memcheck_k32.log', 'new DeltaPage')
    del_delta_cnt = count('results/memcheck_k32.log', 'delete DeltaPage')
    print(f"new DeltaPage: {new_delta_cnt}")
    print(f"delete DeltaPage: {del_delta_cnt}")
    print(f"leak DeltaPage: {new_delta_cnt-del_delta_cnt}, {(1-del_delta_cnt/new_delta_cnt)*100} %")
    print(f"new BasePage: {new_base_cnt}")
    print(f"delete BasePage: {del_base_cnt}")
    print(f"leak BasePage: {new_base_cnt-del_base_cnt}, {(1-del_base_cnt/new_base_cnt)*100} %")