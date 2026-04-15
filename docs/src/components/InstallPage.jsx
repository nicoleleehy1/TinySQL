import CodeBlock from "./CodeBlock";

export default function InstallPage() {
  const steps = [
    {
      n: 1, title: "Prerequisites",
      desc: "Make sure you have clang++ (or g++) and cmake installed.",
      code: `xcode-select --install   # macOS — installs clang++\nbrew install cmake\n\ncmake --version          # should print 3.x or higher\nclang++ --version`,
    },
    {
      n: 2, title: "Clone the repo",
      code: `git clone https://github.com/nicoleleehy1/TinySQL.git\ncd TinySQL`,
    },
    {
      n: 3, title: "Build",
      code: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release\ncmake --build build`,
    },
    {
      n: 4, title: "Run the REPL",
      desc: "Drop into the interactive shell:",
      code: `./build/mini_sql data/employees.csv data/departments.csv`,
    },
    {
      n: 5, title: "Or run a single query",
      code: `./build/mini_sql data/employees.csv -- \\\n  "SELECT name, salary FROM employees WHERE salary > 70000 ORDER BY salary DESC"`,
    },
    {
      n: 6, title: "REPL commands",
      code: `.tables          — list loaded tables\n.schema <table>  — show columns and inferred types\n.quit            — exit`,
    },
  ];

  return (
    <div style={{ maxWidth: 800, margin: "0 auto", padding: "3rem 2rem" }}>
      <h2 style={{ fontSize: 26, fontWeight: 600, marginBottom: 8 }}>Install</h2>
      <p style={{ fontSize: 14, color: "#888780", marginBottom: 32 }}>
        TinySQL is a C++17 project with no external dependencies. You just need a compiler and CMake.
      </p>
      <div style={{ display: "flex", flexDirection: "column", gap: 28 }}>
        {steps.map(step => (
          <div key={step.n} style={{ display: "flex", gap: 16 }}>
            <div style={{
              width: 30, height: 30, borderRadius: "50%", background: "#f1efea",
              border: "0.5px solid #d3d1c7", display: "flex", alignItems: "center",
              justifyContent: "center", fontSize: 13, fontWeight: 600, flexShrink: 0, marginTop: 2,
            }}>{step.n}</div>
            <div style={{ flex: 1 }}>
              <div style={{ fontSize: 15, fontWeight: 600, marginBottom: 4 }}>{step.title}</div>
              {step.desc && <p style={{ fontSize: 14, color: "#5f5e5a", marginBottom: 6, lineHeight: 1.6 }}>{step.desc}</p>}
              <CodeBlock>{step.code}</CodeBlock>
            </div>
          </div>
        ))}
      </div>
    </div>
  );
}