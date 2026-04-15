import Badge from "./Badge";
import CodeBlock from "./CodeBlock";

const GH = "https://github.com/nicoleleehy1/TinySQL";

export default function DocsPage() {
  const features = [
    { icon: "◈", title: "Full SQL subset", desc: "SELECT, WHERE, JOIN, ORDER BY, LIMIT, GROUP BY, aggregates" },
    { icon: "⟳", title: "Cost-based optimizer", desc: "Picks between nested loop, hash join, and sort-merge join" },
    { icon: "▤", title: "CSV-native", desc: "Loads any CSV with a header row, no schema needed" },
    { icon: "›_", title: "REPL + CLI", desc: "Interactive shell or scriptable single-query mode" },
  ];
  return (
    <div>
      <div style={{ padding: "4rem 2rem 3rem", textAlign: "center", borderBottom: "0.5px solid #e8e6e0" }}>
        <h1 style={{ fontSize: 42, fontWeight: 600, letterSpacing: -1, marginBottom: 12, fontFamily: "'Georgia', serif" }}>TinySQL</h1>
        <p style={{ fontSize: 16, color: "#888780", marginBottom: 20, maxWidth: 480, margin: "0 auto 20px" }}>
          A toy SQL engine that runs SELECT, JOIN, and WHERE queries directly against CSV files.
        </p>
        <div style={{ display: "flex", gap: 8, justifyContent: "center", flexWrap: "wrap", marginBottom: 24 }}>
          <Badge variant="cpp">C++17</Badge>
          <Badge>No dependencies</Badge>
          <Badge>Cost-based optimizer</Badge>
          <Badge>REPL + CLI</Badge>
        </div>
        <div style={{ display: "flex", gap: 10, justifyContent: "center" }}>
          <a href={GH} target="_blank" rel="noreferrer" style={{
            padding: "8px 20px", borderRadius: 8, fontSize: 14, background: "#2c2c2a",
            color: "#fff", textDecoration: "none", border: "none",
          }}>View on GitHub</a>
        </div>
      </div>

      <div style={{ maxWidth: 800, margin: "0 auto", padding: "2.5rem 2rem" }}>
        <h2 style={{ fontSize: 20, fontWeight: 600, marginBottom: 12 }}>What it does</h2>
        <p style={{ fontSize: 14, color: "#5f5e5a", lineHeight: 1.8, marginBottom: 24 }}>
          TinySQL parses and executes SQL queries against CSV files with no database server, no schema file, and no setup.
          Point it at your CSVs and start querying. Column types are inferred automatically, and the optimizer picks the
          best join strategy based on table sizes.
        </p>

        <div style={{ display: "grid", gridTemplateColumns: "repeat(auto-fit, minmax(180px, 1fr))", gap: 12, marginBottom: 32 }}>
          {features.map(f => (
            <div key={f.title} style={{ background: "#fff", border: "0.5px solid #e0ddd6", borderRadius: 12, padding: "1rem 1.25rem" }}>
              <div style={{ fontSize: 20, marginBottom: 8 }}>{f.icon}</div>
              <div style={{ fontSize: 14, fontWeight: 600, marginBottom: 4 }}>{f.title}</div>
              <div style={{ fontSize: 13, color: "#888780", lineHeight: 1.5 }}>{f.desc}</div>
            </div>
          ))}
        </div>

        <h2 style={{ fontSize: 20, fontWeight: 600, marginBottom: 12 }}>SQL support</h2>
        <CodeBlock>{`SELECT name, salary FROM employees WHERE salary > 70000 ORDER BY salary DESC LIMIT 5

SELECT e.name, d.name FROM employees e
  JOIN departments d ON e.department_id = d.id

SELECT department_id, COUNT(*), AVG(salary)
  FROM employees GROUP BY department_id`}</CodeBlock>
      </div>
    </div>
  );
}