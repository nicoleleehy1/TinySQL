import { useState, useCallback } from "react";
import DocsPage from "./components/DocsPage";
import InstallPage from "./components/InstallPage";
import Playground from "./components/Playground";

function parseAndRun(sql) {
  const s = sql.trim().replace(/\s+/g, " ");
  const upper = s.toUpperCase();
  if (!upper.startsWith("SELECT")) throw new Error('Only SELECT queries are supported in the playground');
  const fromIdx = upper.indexOf(" FROM ");
  if (fromIdx === -1) throw new Error("Missing FROM clause");

  const selectPart = s.slice(7, fromIdx).trim();
  let rest = s.slice(fromIdx + 6).trim();

  let joinTable = null, joinAlias = null, joinOn = null;
  let mainTable, mainAlias;
  let whereClause = null, orderCol = null, orderDir = "ASC", limitN = null, groupByCol = null;

  const joinMatch = rest.match(/^(\w+)(?:\s+(\w+))?\s+(?:INNER\s+)?JOIN\s+(\w+)(?:\s+(\w+))?\s+ON\s+([\w.]+)\s*=\s*([\w.]+)(.*)/i);
  if (joinMatch) {
    mainTable = joinMatch[1]; mainAlias = joinMatch[2] || joinMatch[1];
    joinTable = joinMatch[3]; joinAlias = joinMatch[4] || joinMatch[3];
    const lc = joinMatch[5].includes(".") ? joinMatch[5].split(".")[1] : joinMatch[5];
    const rc = joinMatch[6].includes(".") ? joinMatch[6].split(".")[1] : joinMatch[6];
    joinOn = { left: lc, right: rc };
    rest = joinMatch[7].trim();
  } else {
    const parts = rest.split(/\s+/);
    mainTable = parts[0];
    mainAlias = parts[1] && !["WHERE","ORDER","LIMIT","GROUP"].includes(parts[1].toUpperCase()) ? parts[1] : mainTable;
    rest = parts.slice(mainAlias !== mainTable ? 2 : 1).join(" ").trim();
  }

  const whereMatch = rest.match(/WHERE\s+(.*?)(?:\s+GROUP BY|\s+ORDER BY|\s+LIMIT|$)/i);
  if (whereMatch) whereClause = whereMatch[1].trim();
  const groupMatch = rest.match(/GROUP BY\s+(\w+)/i);
  if (groupMatch) groupByCol = groupMatch[1];
  const orderMatch = rest.match(/ORDER BY\s+(\w+)\s*(ASC|DESC)?/i);
  if (orderMatch) { orderCol = orderMatch[1]; orderDir = (orderMatch[2] || "ASC").toUpperCase(); }
  const limitMatch = rest.match(/LIMIT\s+(\d+)/i);
  if (limitMatch) limitN = parseInt(limitMatch[1]);

  const tbl = TABLES[mainTable.toLowerCase()];
  if (!tbl) throw new Error(`Table "${mainTable}" not found`);

  const makeRow = (cols, rawRow, alias) => {
    const obj = {};
    cols.forEach((c, i) => { obj[c] = rawRow[i]; obj[`${alias}.${c}`] = rawRow[i]; });
    return obj;
  };

  let rows = tbl.rows.map(r => makeRow(tbl.cols, r, mainAlias));

  let optimizerNote = null;
  if (joinTable) {
    const jtbl = TABLES[joinTable.toLowerCase()];
    if (!jtbl) throw new Error(`Table "${joinTable}" not found`);
    const strategy = rows.length < 100 && jtbl.rows.length < 100 ? "nested loop" : "hash join";
    optimizerNote = `[optimizer] ${rows.length} × ${jtbl.rows.length} rows → ${strategy} on ${joinOn.left} = ${joinOn.right}`;
    const jrows = jtbl.rows.map(r => makeRow(jtbl.cols, r, joinAlias));
    const joined = [];
    rows.forEach(lr => jrows.forEach(rr => {
      const lv = lr[joinOn.left] ?? lr[`${mainAlias}.${joinOn.left}`];
      const rv = rr[joinOn.right] ?? rr[`${joinAlias}.${joinOn.right}`];
      if (String(lv) === String(rv)) joined.push({ ...lr, ...rr });
    }));
    rows = joined;
  }

  const evalWhere = (row, clause) => {
    if (!clause) return true;
    return clause.split(/\s+OR\s+/i).some(part =>
      part.split(/\s+AND\s+/i).every(cond => {
        const m = cond.trim().match(/([\w.]+)\s*(>=|<=|!=|>|<|=)\s*'?([^']+)'?/);
        if (!m) return true;
        const col = m[1].includes(".") ? m[1].split(".")[1] : m[1];
        const op = m[2], val = m[3].trim();
        const cell = row[col];
        if (cell === undefined) return true;
        const nc = parseFloat(cell), nv = parseFloat(val);
        const num = !isNaN(nc) && !isNaN(nv);
        if (op === "=") return num ? nc === nv : String(cell) === val;
        if (op === "!=") return num ? nc !== nv : String(cell) !== val;
        if (op === ">") return nc > nv;
        if (op === "<") return nc < nv;
        if (op === ">=") return nc >= nv;
        if (op === "<=") return nc <= nv;
        return true;
      })
    );
  };

  rows = rows.filter(r => evalWhere(r, whereClause));

  let outCols, outRows;
  const tokens = selectPart.split(",").map(t => t.trim());

  if (groupByCol) {
    const groups = {};
    rows.forEach(r => { const k = r[groupByCol]; if (!groups[k]) groups[k] = []; groups[k].push(r); });
    outCols = tokens.map(t => t);
    outRows = Object.entries(groups).map(([key, gRows]) =>
      tokens.map(tok => {
        if (tok.toUpperCase() === groupByCol.toUpperCase()) return key;
        const agg = tok.match(/(COUNT|SUM|AVG|MIN|MAX)\((\*|\w+)\)/i);
        if (!agg) return key;
        const fn = agg[1].toUpperCase(), col = agg[2];
        if (fn === "COUNT") return gRows.length;
        const vals = gRows.map(r => parseFloat(r[col])).filter(v => !isNaN(v));
        if (fn === "SUM") return vals.reduce((a, b) => a + b, 0).toFixed(2);
        if (fn === "AVG") return (vals.reduce((a, b) => a + b, 0) / vals.length).toFixed(2);
        if (fn === "MIN") return Math.min(...vals);
        if (fn === "MAX") return Math.max(...vals);
      })
    );
  } else if (tokens[0] === "*") {
    const keys = rows.length ? Object.keys(rows[0]).filter(k => !k.includes(".")) : tbl.cols;
    outCols = keys;
    outRows = rows.map(r => keys.map(c => r[c]));
  } else {
    outCols = tokens.map(t => (t.includes(".") ? t.split(".")[1] : t));
    outRows = rows.map(r => tokens.map(t => {
      const bare = t.includes(".") ? t.split(".")[1] : t;
      return r[t] !== undefined ? r[t] : r[bare];
    }));
  }

  if (orderCol) {
    outRows.sort((a, b) => {
      const ci = outCols.findIndex(c => c.toLowerCase() === orderCol.toLowerCase());
      if (ci === -1) return 0;
      const av = a[ci], bv = b[ci];
      const an = parseFloat(av), bn = parseFloat(bv);
      const cmp = !isNaN(an) && !isNaN(bn) ? an - bn : String(av).localeCompare(String(bv));
      return orderDir === "DESC" ? -cmp : cmp;
    });
  }
  if (limitN !== null) outRows = outRows.slice(0, limitN);

  return { cols: outCols, rows: outRows, optimizerNote };
}

const GH = "https://github.com/nicoleleehy1/TinySQL";

export default function App() {
  const [page, setPage] = useState("docs");

  const navLink = (id, label) => (
    <button onClick={() => setPage(id)} style={{
      fontSize: 14, background: "none", border: "none", cursor: "pointer",
      color: page === id ? "#2c2c2a" : "#888780",
      fontWeight: page === id ? 600 : 400,
      fontFamily: "Inter", padding: "4px 0",
      borderBottom: page === id ? "1.5px solid #2c2c2a" : "1.5px solid transparent",
    }}>{label}</button>
  );

  return (
    <div style={{ minHeight: "100vh", background: "#faf9f6", fontFamily: "'Inter', sans-serif", color: "#2c2c2a" }}>
      <nav style={{ display: "flex", alignItems: "center", justifyContent: "space-between", padding: "0 2rem", height: 52, borderBottom: "0.5px solid #e0ddd6", background: "#faf9f6", position: "sticky", top: 0, zIndex: 10 }}>
        <span style={{ fontSize: 16, fontWeight: 700, letterSpacing: -0.3 }}>TinySQL</span>
        <div style={{ display: "flex", gap: 24 }}>
          {navLink("docs", "Docs")}
          {navLink("install", "Install")}
          {navLink("playground", "Playground")}
          <a href={GH} target="_blank" rel="noreferrer" style={{ fontSize: 14, color: "#888780", textDecoration: "none", display: "flex", alignItems: "center" }}>GitHub ↗</a>
        </div>
      </nav>

      {page === "docs" && <DocsPage />}
      {page === "install" && <InstallPage />}
      {page === "playground" && <Playground />}
    </div>
  );
}
