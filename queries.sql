-- ============================================================
-- Example queries for mini-sql-query engine
-- Run with: ./mini_sql <tables...> -- "<query>"
-- ============================================================


-- 1. SELECT * from a single table
--    ./mini_sql data/employees.csv -- "SELECT * FROM employees"
SELECT * FROM employees


-- 2. Projection + WHERE (numeric comparison)
--    ./mini_sql data/employees.csv -- "SELECT name, salary FROM employees WHERE salary > 70000"
SELECT name, salary FROM employees WHERE salary > 70000


-- 3. WHERE with AND (compound condition)
--    ./mini_sql data/employees.csv -- "SELECT name, salary, hire_year FROM employees WHERE salary >= 65000 AND hire_year <= 2020"
SELECT name, salary, hire_year FROM employees WHERE salary >= 65000 AND hire_year <= 2020


-- 4. WHERE with string equality
--    ./mini_sql data/orders.csv -- "SELECT order_id, product, status FROM orders WHERE status = 'delivered'"
SELECT order_id, product, status FROM orders WHERE status = 'delivered'


-- 5. WHERE with OR
--    ./mini_sql data/orders.csv -- "SELECT order_id, employee_id, status FROM orders WHERE status = 'pending' OR status = 'shipped'"
SELECT order_id, employee_id, status FROM orders WHERE status = 'pending' OR status = 'shipped'


-- 6. INNER JOIN — nested loop (both tables < 100 rows)
--    ./mini_sql data/employees.csv data/departments.csv -- "SELECT employees.name, employees.salary, departments.name, departments.location FROM employees JOIN departments ON employees.department_id = departments.id"
SELECT employees.name, employees.salary, departments.name, departments.location
FROM employees
JOIN departments ON employees.department_id = departments.id


-- 7. JOIN + WHERE
--    ./mini_sql data/employees.csv data/departments.csv -- "SELECT employees.name, employees.salary, departments.name FROM employees JOIN departments ON employees.department_id = departments.id WHERE departments.name = 'Engineering'"
SELECT employees.name, employees.salary, departments.name
FROM employees
JOIN departments ON employees.department_id = departments.id
WHERE departments.name = 'Engineering'


-- 8. Three-column JOIN projection with alias + WHERE on salary
--    ./mini_sql data/employees.csv data/orders.csv -- "SELECT e.name, o.product, o.amount FROM employees e JOIN orders o ON e.id = o.employee_id WHERE o.amount > 400"
SELECT e.name, o.product, o.amount
FROM employees e
JOIN orders o ON e.id = o.employee_id
WHERE o.amount > 400


-- 9. Employees in a specific salary band
--    ./mini_sql data/employees.csv -- "SELECT name, salary FROM employees WHERE salary >= 60000 AND salary <= 80000"
SELECT name, salary FROM employees WHERE salary >= 60000 AND salary <= 80000


-- 10. Full order-employee join — tests hash join at larger scale
--     ./mini_sql data/employees.csv data/orders.csv -- "SELECT e.name, o.order_id, o.product, o.status FROM employees e JOIN orders o ON e.id = o.employee_id WHERE o.status != 'pending'"
SELECT e.name, o.order_id, o.product, o.status
FROM employees e
JOIN orders o ON e.id = o.employee_id
WHERE o.status != 'pending'


-- ============================================================
-- ORDER BY / LIMIT / Aggregates
-- ============================================================

-- 11. Global aggregates — no GROUP BY, collapses all rows to one
--     ./mini_sql data/employees.csv -- "SELECT COUNT(*), SUM(salary), AVG(salary), MIN(salary), MAX(salary) FROM employees"
SELECT COUNT(*), SUM(salary), AVG(salary), MIN(salary), MAX(salary)
FROM employees


-- 12. GROUP BY with COUNT and AVG per department
--     ./mini_sql data/employees.csv -- "SELECT department_id, COUNT(*), AVG(salary) FROM employees GROUP BY department_id"
SELECT department_id, COUNT(*), AVG(salary)
FROM employees
GROUP BY department_id


-- 13. ORDER BY salary descending
--     ./mini_sql data/employees.csv -- "SELECT name, salary FROM employees ORDER BY salary DESC"
SELECT name, salary
FROM employees
ORDER BY salary DESC


-- 14. ORDER BY + LIMIT — top 3 earners
--     ./mini_sql data/employees.csv -- "SELECT name, salary FROM employees ORDER BY salary DESC LIMIT 3"
SELECT name, salary
FROM employees
ORDER BY salary DESC
LIMIT 3


-- 15. GROUP BY + ORDER BY aggregate — departments ranked by average salary
--     ./mini_sql data/employees.csv -- "SELECT department_id, COUNT(*), AVG(salary) FROM employees GROUP BY department_id ORDER BY AVG(salary) DESC"
SELECT department_id, COUNT(*), AVG(salary)
FROM employees
GROUP BY department_id
ORDER BY AVG(salary) DESC


-- ============================================================
-- Sort-merge join (requires data/customers.csv + data/purchases.csv,
-- each 120 rows — both above the 100-row threshold, and the key
-- column 'customer_id' ends in '_id', triggering the heuristic)
-- ============================================================

-- 16. Sort-merge join: customer purchases — top 10 by customer id
--     ./mini_sql data/customers.csv data/purchases.csv -- "SELECT customers.id, customers.name, purchases.product, purchases.price FROM customers JOIN purchases ON customers.id = purchases.customer_id ORDER BY customers.id LIMIT 10"
SELECT customers.id, customers.name, purchases.product, purchases.price
FROM customers
JOIN purchases ON customers.id = purchases.customer_id
ORDER BY customers.id
LIMIT 10


-- 17. Sort-merge join + GROUP BY + WHERE: cities ranked by high-value purchase count
--     ./mini_sql data/customers.csv data/purchases.csv -- "SELECT customers.city, COUNT(*) FROM customers JOIN purchases ON customers.id = purchases.customer_id WHERE purchases.price > 1000 GROUP BY customers.city ORDER BY COUNT(*) DESC"
SELECT customers.city, COUNT(*)
FROM customers
JOIN purchases ON customers.id = purchases.customer_id
WHERE purchases.price > 1000
GROUP BY customers.city
ORDER BY COUNT(*) DESC
