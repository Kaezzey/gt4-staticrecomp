// Verify file-backed PT_LOAD bytes after Ghidra imports the reference ELF.
// Run with -postScript VerifyReferenceImport.java <reference ELF path>.
// @category GT4Recomp

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Arrays;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.mem.MemoryBlock;

public class VerifyReferenceImport extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] arguments = getScriptArgs();
        if (arguments.length != 1) {
            throw new IllegalArgumentException("Supply the original reference ELF path");
        }
        byte[] referenceBytes = Files.readAllBytes(Path.of(arguments[0]));
        ByteBuffer header = ByteBuffer.wrap(referenceBytes).order(ByteOrder.LITTLE_ENDIAN);
        long entryAddress = Integer.toUnsignedLong(header.getInt(24));
        int tableOffset = header.getInt(28);
        int recordSize = Short.toUnsignedInt(header.getShort(42));
        int recordCount = Short.toUnsignedInt(header.getShort(44));
        println("M3_LANGUAGE " + currentProgram.getLanguageID());

        for (MemoryBlock block : currentProgram.getMemory().getBlocks()) {
            println("M3_BLOCK " + block.getName() + " " + block.getStart()
                + " size=" + block.getSize() + " initialized=" + block.isInitialized());
        }

        int verifiedSegments = 0;
        for (int index = 0; index < recordCount; index++) {
            int recordOffset = tableOffset + index * recordSize;
            if (header.getInt(recordOffset) != 1) {
                continue; // Only PT_LOAD describes loadable segment bytes.
            }
            int fileOffset = header.getInt(recordOffset + 4);
            long guestAddress = Integer.toUnsignedLong(header.getInt(recordOffset + 8));
            int fileSize = header.getInt(recordOffset + 16);
            byte[] expected = Arrays.copyOfRange(referenceBytes, fileOffset, fileOffset + fileSize);
            byte[] imported = new byte[fileSize];
            int bytesRead = currentProgram.getMemory().getBytes(toAddr(guestAddress), imported);
            if (bytesRead != fileSize || !Arrays.equals(expected, imported)) {
                throw new IllegalStateException("Imported bytes differ at " + toAddr(guestAddress));
            }
            println("M3_LOAD_MATCH address=" + toAddr(guestAddress) + " bytes=" + fileSize);
            verifiedSegments++;
        }
        if (verifiedSegments != 2) {
            throw new IllegalStateException("Expected two loadable segments");
        }

        Address entry = toAddr(entryAddress);
        disassemble(entry);
        Instruction instruction = getInstructionAt(entry);
        if (instruction == null) {
            throw new IllegalStateException("Ghidra could not disassemble the entry instruction");
        }
        println("M3_ENTRY " + entry + " " + instruction);
        println("M3_IMPORT_VERIFIED");
    }
}
