// Independent base-MIPS decoding of original synthetic M5 examples.
// Run as a postScript on any little-endian MIPS program, with analysis disabled.
// The isolated fixture block is created inside the temporary Ghidra project.
// @category GT4Recomp

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Instruction;

public class VerifyDecoderSamples extends GhidraScript {
    @Override
    public void run() throws Exception {
        if (!currentProgram.getLanguageID().toString().equals("MIPS:LE:64:64-32addr")) {
            throw new IllegalStateException("Expected the documented M5 MIPS language");
        }
        int[] words = {
            0x27bdfff0, 0x012a4021, 0x012a4023, 0x012a4024,
            0x012a4025, 0x012a4026, 0x3c081234, 0x8fa80010,
            0xafa80010, 0x11090003, 0x1509fffe, 0x08040000,
            0x0c040000, 0x03e00008, 0x00094100, 0x00094102
        };
        String[] mnemonics = {
            "addiu", "addu", "subu", "and", "or", "xor", "lui", "lw",
            "sw", "beq", "bne", "j", "jal", "jr", "sll", "srl"
        };
        // Hand-derived operands in Ghidra 12.1.3's display syntax. Branch
        // addresses use each sample's PC + 4 + signed immediate * 4.
        String[] operands = {
            "sp,sp,-0x10", "t0,t1,t2", "t0,t1,t2", "t0,t1,t2",
            "t0,t1,t2", "t0,t1,t2", "t0,0x1234", "t0,0x10(sp)",
            "t0,0x10(sp)", "t0,t1,0x010000a0", "t0,t1,0x0100009c",
            "0x00100000", "0x00100000", "ra", "t0,t1,0x4", "t0,t1,0x4"
        };
        Address start = toAddr(0x01000000);
        // Sixteen bytes per sample leave space for zero/NOP delay slots.
        // A branch may reach another sample; every word is written first.
        currentProgram.getMemory().createInitializedBlock(
            "m5_synthetic", start, words.length * 16L, (byte) 0, monitor, false);
        for (int index = 0; index < words.length; index++) {
            Address address = start.add(index * 16L);
            currentProgram.getMemory().setInt(address, words[index]);
        }
        for (int index = 0; index < words.length; index++) {
            Address address = start.add(index * 16L);
            disassemble(address);
            Instruction instruction = getInstructionAt(address);
            if (instruction == null || !instruction.getMnemonicString().equalsIgnoreCase(mnemonics[index])) {
                throw new IllegalStateException("Decode mismatch at " + address + ": " + instruction);
            }
            String expected = mnemonics[index] + " " + operands[index];
            if (!instruction.toString().equalsIgnoreCase(expected)) {
                throw new IllegalStateException("Operand mismatch: expected " + expected
                    + ", observed " + instruction);
            }
            println(String.format("M5_SAMPLE %08x %s %s", words[index], address, instruction));
        }
        println("M5_DECODER_SAMPLES_VERIFIED count=" + words.length);
    }
}
