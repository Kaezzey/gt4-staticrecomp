// Compare an M6 listing with independent decoding of the imported native ELF.
// Usage: -postScript CompareDisassembly.java <listing.txt> <comparison.tsv>
// Game words and assembly are written only to the caller's private report.
// @category GT4Recomp

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import ghidra.app.script.GhidraScript;
import ghidra.app.util.PseudoDisassembler;
import ghidra.app.util.PseudoInstruction;
import ghidra.program.model.address.Address;

public class CompareDisassembly extends GhidraScript {
    private String normalize(String assembly) {
        String normalized = assembly.toLowerCase(Locale.ROOT).replaceAll("\\s+", "");
        Matcher hexadecimal = Pattern.compile("0x[0-9a-f]+").matcher(normalized);
        StringBuilder result = new StringBuilder();
        while (hexadecimal.find()) {
            String value = hexadecimal.group().substring(2);
            hexadecimal.appendReplacement(result, "0x" + Long.toHexString(Long.parseLong(value, 16)));
        }
        hexadecimal.appendTail(result);
        return result.toString();
    }

    private String expandAlias(PseudoInstruction instruction, int word) {
        String name = instruction.getMnemonicString().toLowerCase(Locale.ROOT);
        List<String> operands = new ArrayList<>();
        for (int index = 0; index < instruction.getNumOperands(); index++) {
            operands.add(instruction.getDefaultOperandRepresentation(index));
        }
        if (name.equals("nop") && word == 0) {
            return "sll zero,zero,0x0";
        }
        if (name.equals("li") && operands.size() == 2) {
            int opcode = word >>> 26;
            if (opcode == 9 || opcode == 13) {
                return (opcode == 9 ? "addiu " : "ori ")
                    + operands.get(0) + ",zero," + operands.get(1);
            }
        }
        if ((name.equals("beqz") || name.equals("bnez")) && operands.size() == 2) {
            return (name.equals("beqz") ? "beq " : "bne ")
                + operands.get(0) + ",zero," + operands.get(1);
        }
        if (name.equals("b") && operands.size() == 1 && (word >>> 26) == 4) {
            return "beq zero,zero," + operands.get(0);
        }
        return instruction.toString();
    }

    @Override
    public void run() throws Exception {
        String[] arguments = getScriptArgs();
        if (arguments.length != 2) {
            throw new IllegalArgumentException("Supply listing and private comparison output paths");
        }
        if (!currentProgram.getLanguageID().toString().equals("MIPS:LE:64:64-32addr")) {
            throw new IllegalStateException("Expected the documented little-endian MIPS language");
        }
        if (!currentProgram.getExecutableSHA256().equals(
                "10f82e2231a51404b95682ed3ea81171100a1af2fefdeed3391943016c7c935c")) {
            throw new IllegalStateException("Import the pinned M4 native analysis ELF");
        }
        PseudoDisassembler disassembler = new PseudoDisassembler(currentProgram);
        List<String> report = new ArrayList<>();
        report.add("address\tword\tours\tghidra\tresult");
        int matched = 0;
        int nonNopMatched = 0;
        int unsupported = 0;
        int mismatched = 0;
        Set<Long> addresses = new HashSet<>();
        for (String line : Files.readAllLines(Path.of(arguments[0]))) {
            String[] columns = line.split("\\s+", 3);
            long pc = Long.parseLong(columns[0].replace(":", ""), 16);
            if (!addresses.add(pc)) {
                throw new IllegalStateException("Duplicate sample address");
            }
            int word = (int) Long.parseLong(columns[1], 16);
            Address address = toAddr(pc);
            if (currentProgram.getMemory().getInt(address) != word) {
                throw new IllegalStateException("ELF/listing bytes differ at " + address);
            }
            if (columns[2].startsWith("unsupported ")) {
                unsupported++;
                report.add(String.format("%08x\t%08x\t%s\t\tunsupported", pc, word, columns[2]));
                continue;
            }
            // Decode one instruction without following flows through EE-only opcodes.
            PseudoInstruction reference = disassembler.disassemble(address);
            String referenceText = reference == null ? "<undecodable>" : expandAlias(reference, word);
            boolean agrees = normalize(columns[2]).equals(normalize(referenceText));
            if (agrees) {
                matched++;
                if (word != 0) { nonNopMatched++; }
            } else {
                mismatched++;
            }
            report.add(String.format("%08x\t%08x\t%s\t%s\t%s", pc, word, columns[2],
                reference == null ? "<undecodable>" : reference.toString(), agrees ? "match" : "MISMATCH"));
        }
        Files.write(Path.of(arguments[1]), report);
        println("M6_COMPARISON matched=" + matched + " non_nop=" + nonNopMatched
            + " unsupported=" + unsupported + " mismatched=" + mismatched);
        if (mismatched != 0 || nonNopMatched < 100) {
            throw new IllegalStateException("M6 requires zero mismatches and at least 100 non-NOP matches");
        }
        println("M6_DISASSEMBLY_VERIFIED");
    }
}
