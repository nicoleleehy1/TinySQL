export default function Badge({ children, variant = "default" }) {
  const styles = {
    default: { background: "#f1f1ee", color: "#5f5e5a", border: "0.5px solid #d3d1c7" },
    cpp: { background: "#e6f1fb", color: "#185fa5", border: "none" },
    success: { background: "#eaf3de", color: "#3b6d11", border: "none" },
    error: { background: "#fcebeb", color: "#a32d2d", border: "none" },
    muted: { background: "#f1efea", color: "#888780", border: "none" },
  };
  return (
    <span style={{ fontSize: 11, padding: "3px 9px", borderRadius: 6, fontFamily: "inherit", ...styles[variant] }}>
      {children}
    </span>
  );
}