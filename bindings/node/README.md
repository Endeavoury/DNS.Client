# Ratatoskr for JavaScript and TypeScript

`@ratatoskr/core` calls the canonical native library through Koffi and includes
TypeScript declarations. Set `RATATOSKR_LIBRARY` to an explicit native library,
or install/bundle the platform library in `native/<platform>-<arch>/`.

```ts
import { dns } from "@ratatoskr/core";
const result = await dns.query("example.com", { type: "A" });
```
