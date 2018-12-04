//
// Created by YuShuaiLiu on 2018/11/30.
//
#include "check.h"
#include "unit_tests.h"
#include "sds.h"

START_TEST(test_add) {
  fail_unless(sizeof(struct sdshdr5) == 1, "sdshdr5 size error"); // "error, 2 + 3 != 5"是出错提示信息
}

END_TEST

START_TEST(test_add1) {
  fail_unless(sizeof(struct sdshdr5) == 2, "sdshdr5 size error 2"); // "error, 2 + 3 != 5"是出错提示信息
}

END_TEST

  Suite *sds_test(void) {
  Suite *s = suite_create("sds");       // 建立Suite
  TCase *tc_add = tcase_create("sds");  // 建立测试用例集
  suite_add_tcase(s, tc_add);           // 将测试用例加到Suite中
  tcase_add_test(tc_add, test_add);     // 测试用例加到测试集中
  tcase_add_test(tc_add, test_add1);     // 测试用例加到测试集中
  return s;
}
