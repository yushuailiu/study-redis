//
// Created by YuShuaiLiu on 2018/11/30.
//

#include "stdio.h"
#include "check.h"
#include "unit_tests.h"

START_TEST(int_size) {
  fail_unless(sizeof(int) == 1, "int size is not 1"); // "error, 2 + 3 != 5"是出错提示信息
}

END_TEST

Suite *simple_test(void) {
  Suite *s = suite_create("simple");       // 建立Suite
  TCase *tc_add = tcase_create("simple_test");  // 建立测试用例集
  suite_add_tcase(s, tc_add);           // 将测试用例加到Suite中
  tcase_add_test(tc_add, int_size);     // 测试用例加到测试集中
//  tcase_add_test(tc_add, test_add1);     // 测试用例加到测试集中
  return s;
}