// =====================================================================
//  engine.js - loads the C++ engine (gradebook.wasm) and talks to it.
//
//  The .wasm file was compiled for "WASI", a standard set of operating-
//  system functions (write to a file, read the clock, exit, ...). A browser
//  isn't an operating system, so we hand-write tiny fake versions of the
//  few WASI functions our C++ actually imports. That's the "WASI shim".
//
//  Used by app.js in the browser AND by tests/engine_test.mjs in Node.
// =====================================================================

const ERRNO_SUCCESS = 0;
const ERRNO_BADF = 8;     // "bad file descriptor"
const ERRNO_NOSYS = 52;   // "function not implemented"
const ERRNO_SPIPE = 70;   // "can't seek on this kind of file"

/**
 * Turns the raw bytes of gradebook.wasm into an object with run(command).
 * @param {ArrayBuffer} wasmBytes  e.g. from (await fetch("gradebook.wasm")).arrayBuffer()
 * @param {(text: string) => void} [onStray]  gets anything C++ printed to the
 *        REAL stdout/stderr (normally nothing, because run() redirects cout)
 */
export async function loadEngine(wasmBytes, onStray = () => {}) {
  let memory = null;   // set right after instantiation
  const encoder = new TextEncoder();
  const decoder = new TextDecoder();

  // Fresh views every time: if the C++ side grows its memory, old views
  // of memory.buffer become detached (unusable).
  const dataView = () => new DataView(memory.buffer);
  const bytes = () => new Uint8Array(memory.buffer);

  const wasi = {
    // Write to stdout (fd 1) or stderr (fd 2). "iovs" is a list of
    // (pointer, length) pairs that together make up the text.
    fd_write(fd, iovs, iovsLen, nwrittenPtr) {
      const view = dataView();
      let text = "";
      let written = 0;
      for (let i = 0; i < iovsLen; i++) {
        const ptr = view.getUint32(iovs + i * 8, true);
        const len = view.getUint32(iovs + i * 8 + 4, true);
        text += decoder.decode(bytes().subarray(ptr, ptr + len));
        written += len;
      }
      view.setUint32(nwrittenPtr, written, true);
      if (text) onStray(text, fd);
      return ERRNO_SUCCESS;
    },
    // Reading real stdin: there is none in a browser, so always "end of file".
    fd_read(fd, iovs, iovsLen, nreadPtr) {
      dataView().setUint32(nreadPtr, 0, true);
      return ERRNO_SUCCESS;
    },
    fd_close() {
      return ERRNO_SUCCESS;
    },
    fd_seek() {
      return ERRNO_SPIPE;   // stdin/stdout are streams, not files
    },
    // Describe fds 0-2 as "character devices" (like a terminal).
    fd_fdstat_get(fd, statPtr) {
      if (fd > 2) return ERRNO_BADF;
      const view = dataView();
      view.setUint8(statPtr, 2);                       // fs_filetype = CHARACTER_DEVICE
      view.setUint16(statPtr + 2, 0, true);            // fs_flags
      view.setBigUint64(statPtr + 8, 0xffffffffn, true);   // rights
      view.setBigUint64(statPtr + 16, 0xffffffffn, true);
      return ERRNO_SUCCESS;
    },
    // No environment variables.
    environ_sizes_get(countPtr, sizePtr) {
      const view = dataView();
      view.setUint32(countPtr, 0, true);
      view.setUint32(sizePtr, 0, true);
      return ERRNO_SUCCESS;
    },
    environ_get() {
      return ERRNO_SUCCESS;
    },
    proc_exit(code) {
      throw new Error(`C++ called exit(${code})`);
    },
  };

  // Safety net: if a future build imports a WASI function we didn't write,
  // give it a stub that says "not implemented" instead of failing to load.
  const wasiWithFallback = new Proxy(wasi, {
    get(target, name) {
      if (name in target) return target[name];
      return () => {
        console.warn(`[engine] WASI function "${String(name)}" is not implemented`);
        return ERRNO_NOSYS;
      };
    },
  });

  const { instance } = await WebAssembly.instantiate(wasmBytes, {
    wasi_snapshot_preview1: wasiWithFallback,
  });
  const exp = instance.exports;
  memory = exp.memory;

  // A "reactor" module has no main(). _initialize() runs the C++ global
  // constructors (std::cout itself, and our global GradeBook + CommandShell).
  // Calling run() before this would use objects that were never built.
  exp._initialize();

  /** Sends one command to C++ and returns everything it printed with cout. */
  function run(command) {
    // 1. Copy the command into C++ memory as a NUL-terminated C string.
    const encoded = encoder.encode(command);
    const inPtr = exp.alloc(encoded.length + 1);
    const mem = bytes();
    mem.set(encoded, inPtr);
    mem[inPtr + encoded.length] = 0;

    // 2. Call the C++ run() function.
    let outPtr;
    try {
      outPtr = exp.run(inPtr);
    } finally {
      exp.dealloc(inPtr);
    }

    // 3. Read the answer back (a C string ends at the first 0 byte).
    const after = bytes();   // new view: memory may have grown during run()
    let end = outPtr;
    while (after[end] !== 0) end++;
    return decoder.decode(after.subarray(outPtr, end));
  }

  return { run };
}

/** Quotes text so C++'s std::quoted reads it back exactly ("Ava \"AJ\" Chen"). */
export function quote(text) {
  const oneLine = String(text).replace(/[\r\n]+/g, " ");   // a command is always one line
  return '"' + oneLine.replace(/\\/g, "\\\\").replace(/"/g, '\\"') + '"';
}
