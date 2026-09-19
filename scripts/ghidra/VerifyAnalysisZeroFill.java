// Check the M4 analysis policy's zero-filled tail in the imported database.
// This checks Ghidra loading, not the original console's loader behavior.
// @category GT4Recomp

import ghidra.app.script.GhidraScript;

public class VerifyAnalysisZeroFill extends GhidraScript {
    @Override
    public void run() throws Exception {
        final long start = 0x006d5dfcL;
        final int expectedSize = 0x00800000;
        byte[] importedBytes = new byte[expectedSize];
        int bytesRead = currentProgram.getMemory().getBytes(toAddr(start), importedBytes);
        if (bytesRead != expectedSize) {
            throw new IllegalStateException("Incomplete analysis zero-fill range");
        }
        for (int offset = 0; offset < expectedSize; offset++) {
            if (importedBytes[offset] != 0) {
                throw new IllegalStateException("Nonzero analysis byte at " + toAddr(start + offset));
            }
        }
        println("M4_ZERO_FILL_VERIFIED start=" + toAddr(start) + " bytes=" + expectedSize);
    }
}
