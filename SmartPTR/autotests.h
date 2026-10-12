#ifndef AUTOTESTS_H
#define AUTOTESTS_H

#include <string>

// Подробный вывод внутри тестов (переключается из меню)
extern bool verbose;

void RunAllTests();
void RunWeakPtrDemo();
void RunTest(void (*testFunc)(), const std::string& name);

#endif
