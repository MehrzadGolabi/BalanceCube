%% simulate_realistic_startup.m
% Full 12-second bench lifecycle simulation of BalanceCube_1D_Realistic.slx:
%   - Plant non-idealities: 1.0 deg gearbox backlash, 10 mN*m brush stiction,
%     +-45 deg table rest hard stops, motor-side 275-CPR encoder quantization,
%     45-deg rotated MPU6050 noise + +0.04 rad/s gyro bias.
%   - Controller: Compiled Embedded Coder C99 SIL binary (balanceCubeControllerStep_sil)
%     with boot gyro calibration, 0.42 V stiction deadband compensation, and 4 modes.
%   - Sequence:
%       0.0 - 2.1 s : Resting flat at +45 deg table stop while calibrating gyro bias
%       2.1 - 3.5 s : Hand lifts upright -> engages MODE_BALANCE (2) -> releases
%       5.0 - 5.1 s : +0.04 N*m tap disturbance recovery
%       7.0 - 7.25s : +0.45 N*m hard shove -> trips MODE_FAULT (3) onto +45 deg stop
%       9.0 - 12.0s : Hand lifts upright -> clears fault -> re-arms & balances

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
clear simulinkRealisticMcuHook;
quiet_params = true;
params_1d_cube;

modelName = 'BalanceCube_1D_Realistic';
if ~bdIsLoaded(modelName)
    load_system(fullfile(scriptDir, [modelName, '.slx']));
end

in = Simulink.SimulationInput(modelName);
in = in.setVariable('theta0_deg', 45.0);
in = in.setModelParameter('StopTime', '12.0');
out = sim(in);

sigTh   = out.logsout.get('theta_deg').Values;
sigWr   = out.logsout.get('wheel_rpm').Values;
sigIa   = out.logsout.get('motor_current_A').Values;
sigMode = out.logsout.get('mcu_mode').Values;
sigTau  = out.logsout.get('tau_dist_Nm').Values;

fprintf('=== Realistic Boot-to-Balance Simscape Summary (12.0 s) ===\n');
fprintf('Modes visited: %s\n', mat2str(unique(round(sigMode.Data)).'));
fprintf('t=2.0s (end calib) mode=%d | t=4.5s (balanced) th=%.2f deg, mode=%d\n', ...
    round(interp1(sigMode.Time, sigMode.Data, 2.02, 'previous')), ...
    interp1(sigTh.Time, sigTh.Data, 4.5), ...
    round(interp1(sigMode.Time, sigMode.Data, 4.5, 'previous')));
fprintf('t=8.0s (fallen)    th=%.2f deg, mode=%d | t=12.0s (re-armed) th=%.2f deg, mode=%d\n', ...
    interp1(sigTh.Time, sigTh.Data, 8.0), ...
    round(interp1(sigMode.Time, sigMode.Data, 8.0, 'previous')), ...
    sigTh.Data(end), round(sigMode.Data(end)));
fprintf('Final wheel: %.2f RPM | Peak motor current: %.2f A (Fuse: %.1f A)\n', ...
    sigWr.Data(end), max(abs(sigIa.Data)), I_fuse_limit_A);

assert(ismember(0, round(sigMode.Data)), 'Expected MODE_CALIB (0)');
assert(ismember(1, round(sigMode.Data)), 'Expected MODE_IDLE (1)');
assert(ismember(2, round(sigMode.Data)), 'Expected MODE_BALANCE (2)');
assert(ismember(3, round(sigMode.Data)), 'Expected MODE_FAULT (3)');
assert(round(sigMode.Data(end)) == 2, 'Expected final mode == MODE_BALANCE (2)');
assert(abs(sigTh.Data(end)) < 1.5, 'Expected balanced upright pose at t=12.0 s');

%% Plot 4-Panel Lifecycle Figure
fig = figure('Visible', 'off', 'Position', [80, 80, 1220, 860], 'Color', 'w');
tl = tiledlayout(2, 2, 'TileSpacing', 'compact', 'Padding', 'compact');
title(tl, 'Realistic 1D Balance Cube Lifecycle (C99 SIL + 1.0^\circ Backlash + Stiction + 45^\circ Table Stops)', ...
    'FontWeight', 'bold', 'FontSize', 12.5);

% Panel A: Tilt Angle
ax1 = nexttile;
hold(ax1, 'on'); grid(ax1, 'on');
yline(ax1, table_stop_deg, ':', '+45^\circ Table Stop', 'Color', [0.45 0.45 0.45], 'LineWidth', 1.2);
yline(ax1, catch_limit_deg, '--', '+30^\circ Fault Cutoff', 'Color', [0.8 0.15 0.15], 'LineWidth', 1.1);
yline(ax1, -catch_limit_deg, '--', '-30^\circ Fault Cutoff', 'Color', [0.8 0.15 0.15], 'LineWidth', 1.1);
plot(ax1, sigTh.Time, sigTh.Data, 'Color', [0.05 0.35 0.72], 'LineWidth', 1.8);
xlabel(ax1, 'Time [s]');
ylabel(ax1, 'Tilt Angle \theta [deg]');
title(ax1, 'A. Frame Tilt: Boot @ 45^\circ \rightarrow Lift \rightarrow Tap \rightarrow Knock \rightarrow Re-Arm');
ylim(ax1, [-35, 52]);

% Panel B: MCU State & External Torque
ax2 = nexttile;
hold(ax2, 'on'); grid(ax2, 'on');
yyaxis(ax2, 'left');
stairs(ax2, sigMode.Time, sigMode.Data, 'Color', [0.10 0.58 0.32], 'LineWidth', 2.0, ...
    'DisplayName', 'MCU Mode');
ylabel(ax2, 'MCU State (0=Cal,1=Idle,2=Bal,3=Flt)');
ylim(ax2, [-0.3, 3.5]);
yticks(ax2, 0:3);
yticklabels(ax2, {'0:CALIB', '1:IDLE', '2:BAL', '3:FAULT'});

yyaxis(ax2, 'right');
plot(ax2, sigTau.Time, sigTau.Data, 'Color', [0.82 0.35 0.08], 'LineWidth', 1.3, ...
    'DisplayName', '\tau_{hand/dist}');
ylabel(ax2, 'External Hand / Knock Torque [N\cdotm]');
ylim(ax2, [-1.4, 1.4]);
xlabel(ax2, 'Time [s]');
title(ax2, 'B. C99 State Machine & Virtual Hand / Knockdown Torques');

% Panel C: Motor Shaft Speed (w/ Backlash + Stiction)
ax3 = nexttile;
hold(ax3, 'on'); grid(ax3, 'on');
yline(ax3, 0, ':', 'Color', [0.45 0.45 0.45]);
plot(ax3, sigWr.Time, sigWr.Data, 'Color', [0.45 0.18 0.65], 'LineWidth', 1.6);
xlabel(ax3, 'Time [s]');
ylabel(ax3, 'Motor Shaft Speed [RPM]');
title(ax3, 'C. Motor Speed (1.0^\circ Backlash + 10 mN\cdotm Stiction + 275 CPR)');

% Panel D: Motor Current vs Polyfuse
ax4 = nexttile;
hold(ax4, 'on'); grid(ax4, 'on');
yline(ax4, I_fuse_limit_A, '--', '+3 A Polyfuse', 'Color', [0.8 0.15 0.15], 'LineWidth', 1.2);
yline(ax4, -I_fuse_limit_A, '--', '-3 A Polyfuse', 'Color', [0.8 0.15 0.15], 'LineWidth', 1.2);
plot(ax4, sigIa.Time, sigIa.Data, 'Color', [0.82 0.35 0.08], 'LineWidth', 1.5);
xlabel(ax4, 'Time [s]');
ylabel(ax4, 'Motor Armature Current i_a [A]');
title(ax4, 'D. Armature Current (0.42 V Stiction Comp, < 3 A Fuse)');
ylim(ax4, [-3.5, 3.5]);

outPlot = fullfile(resultsDir, 'realistic_boot_to_balance.png');
exportgraphics(fig, outPlot, 'Resolution', 200);
close(fig);
fprintf('Saved realistic lifecycle plot: %s\n', outPlot);
