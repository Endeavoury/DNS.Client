import test from "node:test";
import assert from "node:assert/strict";
import { abiVersion } from "../src/index.js";

test("loads ABI 1 from the native core", () => assert.equal(abiVersion(), 1));
