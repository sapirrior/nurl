# nurl Project Mandates

## Core Philosophy: The Developer-First HTTP Client
`nurl` is more than just a tool; it is a foundational utility for developers who need to interact with HTTP/HTTPS services via the CLI. Every feature and architectural decision must align with these six pillars:

1.  **Simplicity & Predictability**: The interface should be intuitive and "just work." Users should not need to guess behavior; output and request handling must be consistent across all environments.
2.  **Modernity**: While sticking to the rock-solid HTTP/1.1 protocol, the *interface* and *user experience* should feel modern, utilizing clean terminal output and smart diagnostic logic.
3.  **Maintainability**: Code must be modular, highly readable, and free of "clever" hacks. We prioritize clean abstractions over micro-optimizations that obscure intent.
4.  **Stability**: As a developer tool, `nurl` must be reliable. Error paths must be handled gracefully, and the core engine should be resilient to network edge cases.
5.  **Power**: `nurl` must handle complex developer tasks—streaming, large file transfers with resume support, multipart uploads, and deep request inspection—without losing its simplicity.
6.  **Zero Bloat**: Strictly focused on HTTP/HTTPS. We do not support legacy protocols (Gopher, FTP, etc.) or "extra" features that don't serve the core mission of being a high-quality web client.

## Engineering Standards

### Modular Architecture
The project must strictly adhere to its multi-layered design:
-   **CLI Layer**: Handles argument parsing and subcommand routing.
-   **Engine Layer**: Orchestrates the request lifecycle and manages connection pooling.
-   **Net Layer**: Abstracted socket and proxy management (Winsock/POSIX).
-   **Protocol Layer**: Manual, explicit implementation of HTTP/1.1 for maximum control and predictability.
-   **Utils Layer**: Common helper functions (Base64, URL parsing, Time).

### Developer Experience (DX) & Diagnostics
-   **Smart Errors**: Never return raw, cryptic error codes to the user. Always use the `nurl_diag` system to provide a clear [Error], [Hint], and [Suggestion].
-   **Sensitive Data Protection**: When inspecting requests or printing verbose logs, always redact sensitive headers like `Authorization` or `Cookie` unless explicitly requested otherwise.
-   **Standard Streams**: Maintain perfect compatibility with Unix pipes (`stdin` for request bodies, `stdout` for response payloads).

### Performance & Binary Integrity
-   **Efficiency**: Prioritize efficient HTTPS operations. Leverage the connection pool for redirects and retries to minimize TLS handshake overhead.
-   **Portability**: Ensure the codebase remains natively compatible with Linux, macOS, and Windows. Avoid platform-specific dependencies outside of the `compat` or `net` layers.
-   **Statically Linked**: The build system should favor static linking for critical dependencies (like OpenSSL) to ensure the binary is portable and self-contained.
