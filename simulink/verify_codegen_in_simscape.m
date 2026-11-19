%% verify_codegen_in_simscape.m
% Closes the 3D Simscape Multibody + Electrical plant loop around the actual
% compiled Embedded Coder C99 SIL controller (balanceCubeControllerStep_sil)
% with synthetic 45-deg MPU6050 noise and 275-CPR quadrature quantization,
% comparing directly against the ideal double-precision Simulink LQR baseline.

clear;
clc;
close all;

scriptDir = fileparts(mfilename('fullpath'));
repoRoot = fileparts(scriptDir);
codegenDir = fullfile(repoRoot, 'codegen');
silDir = fullfile(codegenDir, 'sil_build', 'sil');
resultsDir = fullfile(scriptDir, 'results');
if ~exist(resultsDir, 'dir')
    mkdir(resultsDir);
end

if isempty(which('defaultControllerConfig'))
    addpath(codegenDir);
end
if isempty(which('balanceCubeControllerStep_sil'))
    addpath(silDir);
end

assert(~isempty(which('balanceCubeControllerStep_sil')), ...
    'SIL MEX balanceCubeControllerStep_sil not found. Run codegen/build_esp32_controller.m first.');

clear simulinkMcuControllerHook;
quiet_params = true;
params_1d_cube;

%% 1. Simulate Ideal Double-Precision Simulink LQR Baseline (8.0 s)
idealModel = 'BalanceCube_1D';
if ~bdIsLoaded(idealModel)
    load_system(fullfile(scriptDir, [idealModel, '.slx']));
end
in = Simulink.SimulationInput(idealModel);
in = in.setModelParameter('StopTime', '8.0');
out = sim(in);

tIdealTh = out.logsout.get('theta_deg').Values;
tIdealWr = out.logsout.get('wheel_rpm').Values;
tIdealVc = out.logsout.get('V_cmd').Values;
tIdealIa = out.logsout.get('motor_current_A').Values;

%% 2. Simulate Compiled C99 SIL Controller + Quantized Encoder + 45-deg IMU (8.0 s)
mcuModel = 'BalanceCube_1D_MCU';
if ~bdIsLoaded(mcuModel)
    load_system(fullfile(scriptDir, [mcuModel, '.slx']));
end
clear simulinkMcuControllerHook;
in = Simulink.SimulationInput(mcuModel);
in = in.setModelParameter('StopTime', '8.0');
out = sim(in);

tMcuTh = out.logsout.get('theta_deg').Values;
tMcuWr = out.logsout.get('wheel_rpm').Values;
tMcuVc = out.logsout.get('V_cmd').Values;
tMcuIa = out.logsout.get('motor_current_A').Values;

%% 3. Quantitative Equivalence & Stability Checks
fprintf('=== Simscape Closed-Loop Verification: Ideal LQR vs. Compiled C99 SIL ===\n');
fprintf('Ideal Simulink LQR  -> final tilt: %.3f deg | final wheel: %6.2f RPM | peak I: %.2f A\n', ...
    tIdealTh.Data(end), tIdealWr.Data(end), max(abs(tIdealIa.Data)));
fprintf('Compiled C99 SIL    -> final tilt: %.3f deg | final wheel: %6.2f RPM | peak I: %.2f A\n', ...
    tMcuTh.Data(end), tMcuWr.Data(end), max(abs(tMcuIa.Data)));

thMcuOnIdealGrid = interp1(tMcuTh.Time, tMcuTh.Data, tIdealTh.Time, 'linear', 'extrap');
rmseDeg = sqrt(mean((thMcuOnIdealGrid - tIdealTh.Data).^2));
fprintf('Trajectory RMSE (theta_deg, Ideal vs Quantized float32 SIL C99): %.3f deg\n', rmseDeg);

assert(abs(tMcuTh.Data(end)) < 1.0, 'MCU SIL controller failed to hold upright equilibrium');
assert(abs(tMcuWr.Data(end)) < 10.0, 'MCU SIL wheel speed did not return near zero');
assert(max(abs(tMcuIa.Data)) < I_fuse_limit_A, 'MCU SIL exceeded 3 A polyfuse rating');

%% 4. Overlay Comparison Plot
fig = figure('Visible', 'off', 'Position', [80, 80, 1180, 820], 'Color', 'w');
tl = tiledlayout(2, 2, 'TileSpacing', 'compact', 'Padding', 'compact');
title(tl, 'Closed-Loop Simscape Verification: Ideal LQR vs. Generated ESP32 C99 SIL Code', ...
    'FontWeight', 'bold', 'FontSize', 13);

ax1 = nexttile;
hold(ax1, 'on'); grid(ax1, 'on');
xregion(ax1, dist_start_s, dist_end_s, 'FaceColor', [0.95 0.65 0.25], 'FaceAlpha', 0.2, ...
    'DisplayName', '0.04 N\cdotm Tap');
plot(ax1, tIdealTh.Time, tIdealTh.Data, '-', 'Color', [0.05 0.35 0.72], 'LineWidth', 1.8, ...
    'DisplayName', 'Ideal Simulink LQR (double)');
plot(ax1, tMcuTh.Time, tMcuTh.Data, '--', 'Color', [0.85 0.28 0.08], 'LineWidth', 1.6, ...
    'DisplayName', 'C99 SIL (float32 + 275 CPR + 45^\circ IMU)');
xlabel(ax1, 'Time [s]');
ylabel(ax1, 'Tilt Angle \theta [deg]');
title(ax1, sprintf('A. Tilt Angle Overlay (RMSE = %.3f^\\circ)', rmseDeg));
legend(ax1, 'Location', 'northeast', 'FontSize', 8);

ax2 = nexttile;
hold(ax2, 'on'); grid(ax2, 'on');
xregion(ax2, dist_start_s, dist_end_s, 'FaceColor', [0.95 0.65 0.25], 'FaceAlpha', 0.2);
plot(ax2, tIdealWr.Time, tIdealWr.Data, '-', 'Color', [0.05 0.35 0.72], 'LineWidth', 1.8);
plot(ax2, tMcuWr.Time, tMcuWr.Data, '--', 'Color', [0.85 0.28 0.08], 'LineWidth', 1.6);
yline(ax2, 0, ':', 'Color', [0.4 0.4 0.4]);
xlabel(ax2, 'Time [s]');
ylabel(ax2, 'Wheel Speed [RPM]');
title(ax2, 'B. Flywheel Speed (Quantized Encoder + 30 Hz LPF)');

ax3 = nexttile;
hold(ax3, 'on'); grid(ax3, 'on');
xregion(ax3, dist_start_s, dist_end_s, 'FaceColor', [0.95 0.65 0.25], 'FaceAlpha', 0.2);
stairs(ax3, tIdealVc.Time, tIdealVc.Data, '-', 'Color', [0.05 0.35 0.72], 'LineWidth', 1.5);
stairs(ax3, tMcuVc.Time, tMcuVc.Data, '--', 'Color', [0.85 0.28 0.08], 'LineWidth', 1.4);
xlabel(ax3, 'Time [s]');
ylabel(ax3, 'H-Bridge Command V_{cmd} [V]');
title(ax3, 'C. 200 Hz PWM-Equivalent Voltage Command');
ylim(ax3, [-V_max_eff_V - 1.5, V_max_eff_V + 1.5]);

ax4 = nexttile;
hold(ax4, 'on'); grid(ax4, 'on');
xregion(ax4, dist_start_s, dist_end_s, 'FaceColor', [0.95 0.65 0.25], 'FaceAlpha', 0.2);
yline(ax4, I_fuse_limit_A, '--', '3 A Polyfuse', 'Color', [0.75 0.15 0.15]);
plot(ax4, tIdealIa.Time, tIdealIa.Data, '-', 'Color', [0.05 0.35 0.72], 'LineWidth', 1.6);
plot(ax4, tMcuIa.Time, tMcuIa.Data, '--', 'Color', [0.85 0.28 0.08], 'LineWidth', 1.5);
xlabel(ax4, 'Time [s]');
ylabel(ax4, 'Armature Current i_a [A]');
title(ax4, 'D. Armature Current Comparison');

outPlot = fullfile(resultsDir, 'mcu_sil_closed_loop.png');
exportgraphics(fig, outPlot, 'Resolution', 200);
close(fig);
fprintf('Saved SIL overlay plot: %s\n', outPlot);
