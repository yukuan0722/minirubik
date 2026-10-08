param([Parameter(Mandatory = $true)][string]$ToolchainBin)
$ErrorActionPreference = 'Stop'
$taskBin = (Resolve-Path -LiteralPath $ToolchainBin).Path
$env:Path = "$taskBin;$env:Path"
Push-Location $PSScriptRoot
try {
    & "$taskBin\riscv64-unknown-elf-gcc.exe" --version | Set-Content gcc-version.txt
    if ($LASTEXITCODE -ne 0) { throw 'GCC version check failed' }
    # Required optimization, architecture and ABI flags are unchanged.
    & "$taskBin\riscv64-unknown-elf-gcc.exe" -O2 -march=rv32i -mabi=ilp32 -S reference_main.c -o reference_gcc.s
    if ($LASTEXITCODE -ne 0) { throw 'Reference C compilation failed' }
    $taskHelpers = Select-String -Path reference_gcc.s -Pattern '__mulsi3|__divsi3|__udivsi3|__modsi3|__umodsi3'
    if ($taskHelpers) { $taskHelpers; throw 'Arithmetic helper found in reference assembly' }
    & "$taskBin\riscv64-unknown-elf-as.exe" -march=rv32i -mabi=ilp32 reference_gcc.s -o reference_gcc.o
    if ($LASTEXITCODE -ne 0) { throw 'Assembling compiler output failed' }
    & "$taskBin\riscv64-unknown-elf-as.exe" -march=rv32i -mabi=ilp32 reference_start.s -o reference_start.o
    if ($LASTEXITCODE -ne 0) { throw 'Assembling startup failed' }
    & "$taskBin\riscv64-unknown-elf-ld.exe" -m elf32lriscv -T reference.ld -Map 'reference.map' reference_start.o reference_gcc.o -o reference.elf
    if ($LASTEXITCODE -ne 0) { throw 'Reference linking failed' }
    & "$taskBin\riscv64-unknown-elf-readelf.exe" -h -A reference.elf | Tee-Object -FilePath reference-elf-info.txt
    if ($LASTEXITCODE -ne 0) { throw 'ELF inspection failed' }
    & "$taskBin\riscv64-unknown-elf-size.exe" -A reference.elf | Tee-Object -FilePath reference-sections.txt
    if ($LASTEXITCODE -ne 0) { throw 'Section inspection failed' }
    & "$taskBin\riscv64-unknown-elf-objdump.exe" -d reference.elf | Set-Content reference-disassembly.txt
    if ($LASTEXITCODE -ne 0) { throw 'Disassembly failed' }
    $taskForbidden = Select-String -Path reference-disassembly.txt -Pattern '\s(?:mul|mulh|mulhu|mulhsu|div|divu|rem|remu|c\.[a-z0-9.]+)\s|__(?:mul|div|udiv|mod|umod)si3'
    if ($taskForbidden) { $taskForbidden; throw 'Forbidden instruction or arithmetic helper found' }
    Write-Output 'PASS: RV32I reference ELF built; helper and opcode checks passed.'
    Write-Output "Output: $(Join-Path $PSScriptRoot 'reference.elf')"
} finally {
    Pop-Location
}
