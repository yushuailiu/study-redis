//
// Created by YuShuaiLiu on 2018/11/30.
//

#include "unit_tests.h"
#include <stdlib.h>

int main(void) {
  int n;
  SRunner *sr;

  sr = srunner_create(simple_test());

  srunner_add_suite (sr, sds_test());

  srunner_run_all(sr, CK_NORMAL);
  n = srunner_ntests_failed(sr);
  srunner_free(sr);
  return (n == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}