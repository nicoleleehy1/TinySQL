import Badge from "./Badge";
import { useState, useCallback } from "react";

const TABLES = {
  employees: {
    cols: ["id", "name", "salary", "department_id", "age"],
    rows: [
      [1,"Alice",95000,1,34],[2,"Bob",72000,2,28],[3,"Carol",88000,1,41],
      [4,"David",61000,3,25],[5,"Eva",110000,1,38],[6,"Frank",79000,2,31],
      [7,"Grace",54000,4,27],[8,"Hank",93000,3,45],[9,"Iris",67000,4,29],
      [10,"Jake",85000,2,36],
    ],
  },
  departments: {
    cols: ["id", "name", "budget"],
    rows: [
      [1,"Engineering",500000],[2,"Marketing",200000],
      [3,"Operations",300000],[4,"HR",150000],
    ],
  },
  orders: {
    cols: ["id", "employee_id", "amount", "status"],
    rows: [
      [1,1,1200,"closed"],[2,3,850,"open"],[3,5,2100,"closed"],
      [4,2,430,"open"],[5,7,990,"closed"],[6,1,760,"open"],
      [7,4,1500,"closed"],[8,8,670,"open"],[9,6,310,"closed"],[10,9,1800,"open"],
    ],
  },
};

const SAMPLES = {
  select: "SELECT * FROM employees",
  where: "SELECT name, salary FROM employees WHERE salary > 75000",
  join: "SELECT e.name, e.salary, d.name FROM employees e JOIN departments d ON e.department_id = d.id WHERE e.salary > 70000",
  agg: "SELECT department_id, COUNT(*), AVG(salary) FROM employees GROUP BY department_id",
  order: "SELECT name, salary FROM employees ORDER BY salary DESC LIMIT 5",
};

export default function Playground() {
  const [query, setQuery] = useState("SELECT * FROM employees");
  const [result, setResult] = useState(null);
  const [error, setError] = useState(null);
  const [optimizerNote, setOptimizerNote] = useState(null);
  const [rowCount, setRowCount] = useState(null);

  const run = useCallback(() => {
    try {
      const res = parseAndRun(query);
      setResult(res);
      setRowCount(res.rows.length);
      setOptimizerNote(res.optimizerNote);
      setError(null);
    } catch (e) {
      setError(e.message);
      setResult(null);
      setOptimizerNote(null);
      setRowCount(null);
    }
  }, [query]);

  const previewTable = (name) => {
    const q = `SELECT * FROM ${name}`;
    setQuery(q);
    try {
      const res = parseAndRun(q);
      setResult(res); setRowCount(res.rows.length); setOptimizerNote(null); setError(null);
    } catch (e) { setError(e.message); }
  };

  const tableEntries = Object.entries(TABLES);

  return (
    <div style={{ maxWidth: 900, margin: "0 auto", padding: "2.5rem 2rem" }}>
      <h2 style={{ fontSize: 26, fontWeight: 600, marginBottom: 6 }}>Playground</h2>
      <p style={{ fontSize: 14, color: "#888780", marginBottom: 24 }}>
        Try TinySQL in your browser against sample data. This is a JS simulation — install the CLI for full performance.
      </p>

      <div style={{ display: "grid", gridTemplateColumns: "200px 1fr", gap: 16 }}>
        <div style={{ border: "0.5px solid #e0ddd6", borderRadius: 12, overflow: "hidden" }}>
          <div style={{ padding: "8px 14px", fontSize: 11, fontWeight: 600, color: "#888780", background: "#f8f7f4", borderBottom: "0.5px solid #e0ddd6", textTransform: "uppercase", letterSpacing: 0.5 }}>
            Tables
          </div>
          {tableEntries.map(([name, t]) => (
            <div key={name} onClick={() => previewTable(name)} style={{
              padding: "10px 14px", cursor: "pointer", borderBottom: "0.5px solid #f0ede7",
              transition: "background 0.1s",
            }}
              onMouseEnter={e => e.currentTarget.style.background = "#f8f7f4"}
              onMouseLeave={e => e.currentTarget.style.background = "transparent"}
            >
              <div style={{ fontSize: 13, fontWeight: 600, marginBottom: 2 }}>{name}</div>
              <div style={{ fontSize: 11, color: "#888780" }}>{t.rows.length} rows · {t.cols.length} cols</div>
              <div style={{ fontSize: 11, color: "#b4b2a9", fontFamily: "monospace", marginTop: 3 }}>{t.cols.join(", ")}</div>
            </div>
          ))}
        </div>

        <div style={{ display: "flex", flexDirection: "column", gap: 12 }}>
          <div style={{ border: "0.5px solid #e0ddd6", borderRadius: 12, overflow: "hidden" }}>
            <div style={{ display: "flex", alignItems: "center", justifyContent: "space-between", padding: "8px 12px", background: "#f8f7f4", borderBottom: "0.5px solid #e0ddd6" }}>
              <span style={{ fontSize: 12, color: "#888780" }}>SQL query</span>
              <div style={{ display: "flex", gap: 6, flexWrap: "wrap" }}>
                {Object.entries(SAMPLES).map(([k, v]) => (
                  <button key={k} onClick={() => setQuery(v)} style={{
                    fontSize: 11, padding: "2px 8px", borderRadius: 6, border: "0.5px solid #d3d1c7",
                    background: "transparent", cursor: "pointer", color: "#5f5e5a", fontFamily: "inherit",
                  }}>{k}</button>
                ))}
              </div>
            </div>
            <textarea
              value={query}
              onChange={e => setQuery(e.target.value)}
              onKeyDown={e => { if ((e.metaKey || e.ctrlKey) && e.key === "Enter") run(); }}
              spellCheck={false}
              style={{
                width: "100%", border: "none", outline: "none", padding: "12px 14px",
                fontFamily: "ui-monospace, monospace", fontSize: 13, resize: "none", height: 90,
                background: "#fff", color: "#2c2c2a", lineHeight: 1.6,
              }}
            />
            <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center", padding: "8px 12px", background: "#f8f7f4", borderTop: "0.5px solid #e0ddd6" }}>
              <span style={{ fontSize: 11, color: "#b4b2a9" }}>⌘ + Enter to run</span>
              <button onClick={run} style={{
                padding: "7px 18px", borderRadius: 7, fontSize: 13, background: "#2c2c2a",
                color: "#fff", border: "none", cursor: "pointer", fontFamily: "inherit", fontWeight: 500,
              }}>Run query</button>
            </div>
          </div>

          <div style={{ border: "0.5px solid #e0ddd6", borderRadius: 12, overflow: "hidden", minHeight: 180 }}>
            <div style={{ display: "flex", alignItems: "center", justifyContent: "space-between", padding: "8px 14px", background: "#f8f7f4", borderBottom: "0.5px solid #e0ddd6" }}>
              <span style={{ fontSize: 12, color: "#888780" }}>Results</span>
              {error && <Badge variant="error">error</Badge>}
              {!error && rowCount !== null && <Badge variant="success">{rowCount} row{rowCount !== 1 ? "s" : ""}</Badge>}
              {!error && rowCount === null && <Badge variant="muted">ready</Badge>}
            </div>

            {!result && !error && (
              <div style={{ display: "flex", alignItems: "center", justifyContent: "center", height: 120, fontSize: 13, color: "#b4b2a9" }}>
                Run a query to see results
              </div>
            )}
            {error && (
              <div style={{ padding: "16px 14px", fontSize: 13, color: "#a32d2d", fontFamily: "monospace" }}>
                {error}
              </div>
            )}
            {result && (
              <div style={{ overflowX: "auto" }}>
                <table style={{ width: "100%", borderCollapse: "collapse", fontSize: 13 }}>
                  <thead>
                    <tr>
                      {result.cols.map(c => (
                        <th key={c} style={{ padding: "8px 14px", textAlign: "left", fontWeight: 600, fontSize: 12, color: "#888780", borderBottom: "0.5px solid #e0ddd6", background: "#f8f7f4", whiteSpace: "nowrap" }}>{c}</th>
                      ))}
                    </tr>
                  </thead>
                  <tbody>
                    {result.rows.length === 0 ? (
                      <tr><td colSpan={result.cols.length} style={{ textAlign: "center", padding: 20, color: "#b4b2a9" }}>No rows returned</td></tr>
                    ) : result.rows.map((row, i) => (
                      <tr key={i} onMouseEnter={e => e.currentTarget.style.background = "#faf9f6"} onMouseLeave={e => e.currentTarget.style.background = "transparent"}>
                        {row.map((v, j) => (
                          <td key={j} style={{ padding: "8px 14px", borderBottom: "0.5px solid #f0ede7", color: "#2c2c2a" }}>{v ?? ""}</td>
                        ))}
                      </tr>
                    ))}
                  </tbody>
                </table>
              </div>
            )}
            {optimizerNote && (
              <div style={{ padding: "7px 14px", fontSize: 11, color: "#888780", borderTop: "0.5px solid #e0ddd6", background: "#f8f7f4", fontFamily: "monospace" }}>
                {optimizerNote}
              </div>
            )}
          </div>
        </div>
      </div>
    </div>
  );
}