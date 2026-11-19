%% build_esp32_controller.m
% End-to-end MATLAB Coder workflow for the 1D Balance Cube controller:
%   1. Static screening with coder.screener
%   2. Interpreted MATLAB behavioral verification
%   3. MEX build + coder.runTest equivalence check
%   4. Embedded Coder SIL build + coder.runTest equivalence check
%   5. Production bare-metal 32-bit C99 export to ../firmware/balance_controller_c/

clear;
clc;

codegenDir = fileparts(mfilename('fullpath'));
repoRoot = fileparts(codegenDir);
cd(codegenDir);

%% 1. Screen Function for Codegen Readiness
fprintf('--- Step 1: Screening balanceCubeControllerStep ---\n');
res = coder.screener('balanceCubeControllerStep');
disp(res.UnsupportedCalls);
disp(res.Messages);
assert(isempty(res.UnsupportedCalls), 'Unsupported calls found by coder.screener!');
assert(isempty(res.Messages), 'Warnings/messages found by coder.screener!');

%% 2. Baseline Interpreted MATLAB Verification
fprintf('--- Step 2: Running MATLAB Behavioral Test ---\n');
run(fullfile(codegenDir, 'test_balanceCubeControllerStep.m'));
fprintf('Interpreted MATLAB test passed.\n');

%% 3. Define Fixed Input Types
cfgSample = defaultControllerConfig();
inputTypes = { ...
    coder.typeof(single(0)), ...
    coder.typeof(single(0)), ...
    coder.typeof(single(0)), ...
    coder.typeof(int32(0)), ...
    coder.typeof(false), ...
    coder.typeof(cfgSample)};

%% 4. Generate MEX & Verify via coder.runTest
fprintf('--- Step 3: Building MEX & Running coder.runTest ---\n');
cfgMex = coder.config('mex');
cfgMex.TargetLang = 'C';
mexOutDir = fullfile(codegenDir, 'mex_build');
codegen('balanceCubeControllerStep', '-config', cfgMex, '-args', inputTypes, '-d', mexOutDir);
coder.runTest('test_balanceCubeControllerStep', 'balanceCubeControllerStep');
fprintf('MEX coder.runTest passed.\n');

%% 5. Generate Embedded Coder SIL Library & Verify via coder.runTest
fprintf('--- Step 4: Building SIL Library & Running coder.runTest ---\n');
cfgSil = coder.config('lib', 'ecoder', true);
cfgSil.TargetLang = 'C';
cfgSil.EnableDynamicMemoryAllocation = false;
cfgSil.EnableRuntimeRecursion = false;
cfgSil.EnableMemcpy = false;
cfgSil.SupportNonFinite = false;
cfgSil.StackUsageMax = 512;
cfgSil.VerificationMode = 'SIL';

silOutDir = fullfile(codegenDir, 'sil_build');
codegen('balanceCubeControllerStep', '-config', cfgSil, '-args', inputTypes, '-d', silOutDir);
coder.runTest('test_balanceCubeControllerStep', 'balanceCubeControllerStep', 'balanceCubeControllerStep_sil');
fprintf('SIL coder.runTest passed.\n');

%% 6. Generate Production Bare-Metal 32-bit C99 Code for ESP32
fprintf('--- Step 5: Generating ESP32 Bare-Metal Static C99 Source ---\n');
cfgEsp = coder.config('lib', 'ecoder', true);
cfgEsp.TargetLang = 'C';
cfgEsp.GenCodeOnly = true;
cfgEsp.GenerateExampleMain = 'DoNotGenerate';
cfgEsp.EnableDynamicMemoryAllocation = false;
cfgEsp.EnableRuntimeRecursion = false;
cfgEsp.EnableMemcpy = false;
cfgEsp.SupportNonFinite = false;
cfgEsp.StackUsageMax = 512;
cfgEsp.HardwareImplementation.ProdHWDeviceType = 'Generic->Custom';

espOutDir = fullfile(repoRoot, 'firmware', 'balance_controller_c');
if exist(espOutDir, 'dir')
    rmdir(espOutDir, 's');
end
codegen('balanceCubeControllerStep', '-config', cfgEsp, '-args', inputTypes, '-d', espOutDir);

%% 7. Audit Generated C99 Files for Dynamic Allocation & BalanceCubeConfig_T
cFiles = [dir(fullfile(espOutDir, '*.c')); dir(fullfile(espOutDir, '*.h'))];
fprintf('\nGenerated C99 files in %s:\n', espOutDir);
heapHits = 0;
foundStructType = false;
for k = 1:numel(cFiles)
    fpath = fullfile(cFiles(k).folder, cFiles(k).name);
    txt = fileread(fpath);
    if contains(txt, 'BalanceCubeConfig_T')
        foundStructType = true;
    end
    if ~isempty(regexp(txt, '\b(malloc|calloc|realloc|free)\s*\(', 'once'))
        heapHits = heapHits + 1;
        fprintf('  [WARN] Heap call in %s\n', cFiles(k).name);
    end
    fprintf('  - %-42s (%6d bytes)\n', cFiles(k).name, cFiles(k).bytes);
end

assert(foundStructType, 'Expected BalanceCubeConfig_T typedef in generated headers!');
assert(heapHits == 0, 'Dynamic memory allocation detected in generated C code!');
fprintf('\nAudit passed: BalanceCubeConfig_T present, 0 dynamic memory calls.\n');
