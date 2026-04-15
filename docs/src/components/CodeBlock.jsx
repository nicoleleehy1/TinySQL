export default function CodeBlock({ children }) {
  return (
    <pre style={{
      background: "#f8f7f4", border: "0.5px solid #d3d1c7", borderRadius: 8,
      padding: "12px 14px", fontFamily: "ui-monospace, monospace", fontSize: 13,
      overflowX: "auto", margin: "8px 0", color: "#2c2c2a", lineHeight: 1.6,
    }}>
      {children}
    </pre>
  );
}