#ifndef EMPLOYEE_H
#define EMPLOYEE_H

#include <string>
#include "SharedPtr.h"

// ============================================================
// Подопытные классы для доп. задания: циклические ссылки и WeakPtr.
//
// Два почти одинаковых класса. Разница ровно в одном поле:
//   EmployeeCycle::colleague — SharedPtr  → цикл, объекты не удаляются
//   Employee::colleague      — WeakPtr    → цикл разорван, всё удаляется
//
// Счётчики живых экземпляров — как у Probe, чтобы утечка была видна числом.
// ============================================================

// --- вариант с СИЛЬНОЙ ссылкой: демонстрация проблемы ---
struct EmployeeCycle {
    static inline int aliveCount = 0;

    std::string name;
    SharedPtr<EmployeeCycle> colleague;      // сильная ссылка на коллегу

    explicit EmployeeCycle(const std::string& n) : name(n) { aliveCount++; }
    ~EmployeeCycle() { aliveCount--; }
};

// --- вариант со СЛАБОЙ ссылкой: решение ---
struct Employee {
    static inline int aliveCount = 0;

    std::string name;
    WeakPtr<Employee> colleague;             // слабая ссылка на коллегу

    explicit Employee(const std::string& n) : name(n) { aliveCount++; }
    ~Employee() { aliveCount--; }
};

#endif
