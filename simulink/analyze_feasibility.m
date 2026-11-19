%% analyze_feasibility.m
% Nonlinear 3D Simscape Multibody + Electrical closed-loop simulation and
% multi-parameter physical feasibility trade study for the 1-Wheel Balance Cube.
%
% Generates:
%   1. results/closed_loop_response.png       - 8.0 s baseline response + tap recovery
%   2. results/parametric_feasibility_sweeps.png - Motor, flywheel, square & driver sweeps

clear;
clc;
close all;

scriptDir = fileparts(mfilename('fullpath'));
resultsDir = fullfile(scriptDir, 'results');
if ~exist(resultsDir, 'dir')
    mkdir(resultsDir);
end

modelName = 'BalanceCube_1D';
modelPath = fullfile(scriptDir, [modelName, '.slx']);
if ~bdIsLoaded(modelName)
    load_system(modelPath);
end

%% 1. Baseline Configuration & Nonlinear 3D Simscape Closed-Loop Simulation
quiet_params = false; %#ok<NASGU>
params_1d_cube;

in = Simulink.SimulationInput(modelName);
in = in.setModelParameter('StopTime', '8.0');
out = sim(in);

sigTh = out.logsout.get('theta_deg').Values;
sigThd = out.logsout.get('theta_dot_rads').Values;
sigWr = out.logsout.get('wheel_rpm').Values;
sigVc = out.logsout.get('V_cmd').Values;
sigIa = out.logsout.get('motor_current_A').Values;

tBase = sigTh.Time;
thBase = sigTh.Data;
thdBase = sigThd.Data;
tWr = sigWr.Time;
wrBase = sigWr.Data;
tVc = sigVc.Time;
vcBase = sigVc.Data;
tIa = sigIa.Time;
iaBase = sigIa.Data;

% Verify closed-loop criteria
fprintf('\n--- Baseline Nonlinear Simscape Verification (8.0 s) ---\n');
fprintf('Initial tilt: %.2f deg | Pre-tap (2.0 s): %.3f deg | Tap peak: %.3f deg | Final: %.3f deg\n', ...
    thBase(1), interp1(tBase, thBase, 2.0), ...
    max(abs(thBase(tBase >= 2.49 & tBase <= 4.0))), thBase(end));
fprintf('Final |theta_dot|: %.5f rad/s | Peak wheel: %.1f RPM | Final wheel: %.2f RPM\n', ...
    abs(thdBase(end)), max(abs(wrBase)), wrBase(end));
fprintf('Peak voltage: %.2f V | Peak current: %.2f A (Fuse limit: %.1f A)\n', ...
    max(abs(vcBase)), max(abs(iaBase)), I_fuse_limit_A);

% Plot 4-panel Baseline Closed-Loop Response
fig1 = figure('Visible', 'off', 'Position', [80, 80, 1180, 820], 'Color', 'w');
tl1 = tiledlayout(2, 2, 'TileSpacing', 'compact', 'Padding', 'compact');
title(tl1, sprintf('1D Balance Cube Closed-Loop Simscape Response (140 mm Acrylic, 90 mm Brass Wheel, L298N 12V)'), ...
    'FontWeight', 'bold', 'FontSize', 13);

% Panel 1: Tilt angle
ax1 = nexttile;
hold(ax1, 'on'); grid(ax1, 'on');
yline(ax1, rad2deg(atan2(com_imbalance_offset_m, L_center_m)), '--', ...
    sprintf('CoM Trim Eq (%.2f^\\circ)', rad2deg(atan2(com_imbalance_offset_m, L_center_m))), ...
    'Color', [0.4 0.4 0.4], 'LineWidth', 1.2, 'LabelHorizontalAlignment', 'left', ...
    'DisplayName', 'CoM Equilibrium');
xregion(ax1, dist_start_s, dist_end_s, 'FaceColor', [0.95 0.65 0.25], 'FaceAlpha', 0.2, ...
    'DisplayName', '0.04 N\cdotm Tap');
plot(ax1, tBase, thBase, 'Color', [0.05 0.35 0.72], 'LineWidth', 1.8, 'DisplayName', '\theta(t)');
xlabel(ax1, 'Time [s]');
ylabel(ax1, 'Tilt Angle \theta [deg]');
title(ax1, 'A. Frame Tilt Recovery & Tap Disturbance Rejection');
legend(ax1, 'Location', 'northeast');

% Panel 2: Reaction Wheel Speed
ax2 = nexttile;
hold(ax2, 'on'); grid(ax2, 'on');
yline(ax2, w_max_eff_rpm, ':', sprintf('+No-Load Ceiling (+%.0f RPM)', w_max_eff_rpm), ...
    'Color', [0.75 0.15 0.15], 'LineWidth', 1.2);
yline(ax2, 0, '--', 'Color', [0.5 0.5 0.5], 'LineWidth', 1.0);
xregion(ax2, dist_start_s, dist_end_s, 'FaceColor', [0.95 0.65 0.25], 'FaceAlpha', 0.2);
plot(ax2, tWr, wrBase, 'Color', [0.10 0.55 0.32], 'LineWidth', 1.8);
xlabel(ax2, 'Time [s]');
ylabel(ax2, 'Wheel Speed [RPM]');
title(ax2, 'B. Flywheel Speed (Decays to 0 RPM via 4th State)');
ylim(ax2, [-250, max(300, max(wrBase)*1.25)]);

% Panel 3: Commanded Voltage
ax3 = nexttile;
hold(ax3, 'on'); grid(ax3, 'on');
yline(ax3, V_max_eff_V, '--', sprintf('L298N +Rail (+%.1f V)', V_max_eff_V), ...
    'Color', [0.75 0.15 0.15], 'LineWidth', 1.1);
yline(ax3, -V_max_eff_V, '--', sprintf('L298N -Rail (-%.1f V)', V_max_eff_V), ...
    'Color', [0.75 0.15 0.15], 'LineWidth', 1.1);
xregion(ax3, dist_start_s, dist_end_s, 'FaceColor', [0.95 0.65 0.25], 'FaceAlpha', 0.2);
stairs(ax3, tVc, vcBase, 'Color', [0.55 0.18 0.65], 'LineWidth', 1.6);
xlabel(ax3, 'Time [s]');
ylabel(ax3, 'Driver Voltage V_{cmd} [V]');
title(ax3, 'C. 200 Hz LQR Command Voltage');
ylim(ax3, [-V_max_eff_V - 1.5, V_max_eff_V + 1.5]);

% Panel 4: Armature Current
ax4 = nexttile;
hold(ax4, 'on'); grid(ax4, 'on');
yline(ax4, I_fuse_limit_A, '--', sprintf('+Polyfuse Limit (+%.1f A)', I_fuse_limit_A), ...
    'Color', [0.8 0.15 0.15], 'LineWidth', 1.2);
yline(ax4, -I_fuse_limit_A, '--', sprintf('-Polyfuse Limit (-%.1f A)', I_fuse_limit_A), ...
    'Color', [0.8 0.15 0.15], 'LineWidth', 1.2);
xregion(ax4, dist_start_s, dist_end_s, 'FaceColor', [0.95 0.65 0.25], 'FaceAlpha', 0.2);
plot(ax4, tIa, iaBase, 'Color', [0.82 0.35 0.08], 'LineWidth', 1.6);
xlabel(ax4, 'Time [s]');
ylabel(ax4, 'Armature Current i_a [A]');
title(ax4, 'D. Motor Current vs. 3 A Schematic Polyfuse');
ylim(ax4, [-I_fuse_limit_A - 0.5, I_fuse_limit_A + 0.5]);

outPlot1 = fullfile(resultsDir, 'closed_loop_response.png');
exportgraphics(fig1, outPlot1, 'Resolution', 200);
close(fig1);

%% 2. Sweep 1: Real Off-the-Shelf Motor Comparison (Nonlinear 3D Simscape)
baseCfg = defaultDesignConfig();
motors = [
    struct('name', 'RS-385 Direct (L298N)',       'mass', 0.080, 'Tstall', 0.035, 'rpm', 9000, 'Vdrop', 2.0, 'color', [0.80 0.20 0.20]);
    struct('name', 'JGB37-520 30:1 (L298N)',      'mass', 0.155, 'Tstall', 0.750, 'rpm',  330, 'Vdrop', 2.0, 'color', [0.85 0.55 0.10]);
    struct('name', 'JGB37-520 6.25:1 (L298N)',    'mass', 0.155, 'Tstall', 0.240, 'rpm', 1600, 'Vdrop', 2.0, 'color', [0.05 0.35 0.72]);
    struct('name', 'RS-555 Direct (L298N)',       'mass', 0.220, 'Tstall', 0.260, 'rpm', 4500, 'Vdrop', 2.0, 'color', [0.55 0.18 0.65]);
    struct('name', 'BLDC Gimbal/24H (MOSFET)',    'mass', 0.130, 'Tstall', 0.300, 'rpm', 4000, 'Vdrop', 0.2, 'color', [0.10 0.58 0.32])
];

nMot = numel(motors);
in = repmat(Simulink.SimulationInput(modelName), nMot, 1);
motParams = cell(nMot, 1);
for k = 1:nMot
    cfg = baseCfg;
    cfg.motor_mass_kg = motors(k).mass;
    cfg.motor_T_stall_Nm = motors(k).Tstall;
    cfg.motor_rpm_noload = motors(k).rpm;
    cfg.V_drop_driver_V = motors(k).Vdrop;
    cfg.theta0_deg = 6.0;
    cfg.dist_torque_Nm = 0.0;
    cfg.com_imbalance_offset_m = 0.0;
    p = computeDerivedDesign(cfg);
    motParams{k} = p;
    in(k) = applyParamsToSimInput(Simulink.SimulationInput(modelName), p);
    in(k) = in(k).setModelParameter('StopTime', '4.0');
end
out = sim(in);

fprintf('\n=== Motor Trade-Off Study (140 mm Acrylic, 90 mm Brass Wheel, theta0 = 6 deg) ===\n');
fprintf('%-26s | %6s | %6s | %7s | %8s | %8s | %s\n', ...
    'Motor + Driver', 'Mass', 'I_stl', 'MaxRPM', 'LinThMax', 'SimPeakI', '6-deg Sim Status');
fprintf('%s\n', repmat('-', 1, 92));
simMotorResults = cell(nMot, 1);
for k = 1:nMot
    p = motParams{k};
    th = out(k).logsout.get('theta_deg').Values;
    ia = out(k).logsout.get('motor_current_A').Values;
    recovered = abs(th.Data(end)) < 1.0;
    statusStr = 'FAIL (Falls)';
    if recovered
        if p.I_stall_eff_A > 3.0
            statusStr = 'RECOVER (Trips 3A Fuse/L298N!)';
        else
            statusStr = 'RECOVER (Within 3A Limit)';
        end
    end
    simMotorResults{k} = struct('time', th.Time, 'theta', th.Data, ...
        'peakI', max(abs(ia.Data)), 'recovered', recovered);
    fprintf('%-26s | %4.0fg | %5.2fA | %5.0f   | %6.2f^o | %6.2f A | %s\n', ...
        motors(k).name, motors(k).mass*1e3, p.I_stall_eff_A, p.w_max_eff_rpm, ...
        p.theta_max_recoverable_deg, max(abs(ia.Data)), statusStr);
end

%% 3. Sweep 2: Motor Torque vs. No-Load RPM Contour Map (12V + L298N)
tStallVec = linspace(0.03, 0.80, 50);
rpmVec = linspace(250, 5000, 45);
[TGrid, RGrid] = meshgrid(tStallVec, rpmVec);
thMaxMotGrid = zeros(size(TGrid));
iStallGrid = zeros(size(TGrid));
for r = 1:size(TGrid, 1)
    for c = 1:size(TGrid, 2)
        cfg = baseCfg;
        cfg.motor_T_stall_Nm = TGrid(r, c);
        cfg.motor_rpm_noload = RGrid(r, c);
        p = computeDerivedDesign(cfg);
        thMaxMotGrid(r, c) = p.theta_max_recoverable_deg;
        iStallGrid(r, c) = p.I_stall_eff_A;
    end
end

%% 4. Sweep 3: Flywheel Outer Radius vs. Rim Thickness / Weight + Simscape Checks
radVec = linspace(0.025, 0.065, 41);
rimVec = linspace(0.004, 0.024, 41);
[RadGrid, RimGrid] = meshgrid(radVec, rimVec);
thMaxWheelGrid = zeros(size(RadGrid));
massWheelGrid = zeros(size(RadGrid));
for r = 1:size(RadGrid, 1)
    for c = 1:size(RadGrid, 2)
        cfg = baseCfg;
        cfg.flywheel_outer_radius_m = RadGrid(r, c);
        cfg.flywheel_inner_radius_m = max(0.010, RadGrid(r, c) - 0.012);
        cfg.flywheel_rim_thickness_m = RimGrid(r, c);
        p = computeDerivedDesign(cfg);
        thMaxWheelGrid(r, c) = p.theta_max_recoverable_deg;
        massWheelGrid(r, c) = p.flywheel_mass_kg * 1e3;
    end
end

% 4 Nonlinear Simscape verification points across flywheel sizes @ theta0 = 12 deg
wheelChecks = [
    struct('r_m', 0.030, 't_m', 0.010);
    struct('r_m', 0.035, 't_m', 0.018);
    struct('r_m', 0.045, 't_m', 0.012);
    struct('r_m', 0.055, 't_m', 0.012)
];
in = repmat(Simulink.SimulationInput(modelName), numel(wheelChecks), 1);
for k = 1:numel(wheelChecks)
    cfg = baseCfg;
    cfg.flywheel_outer_radius_m = wheelChecks(k).r_m;
    cfg.flywheel_inner_radius_m = max(0.010, wheelChecks(k).r_m - 0.012);
    cfg.flywheel_rim_thickness_m = wheelChecks(k).t_m;
    cfg.theta0_deg = 12.0;
    cfg.dist_torque_Nm = 0.0;
    cfg.com_imbalance_offset_m = 0.0;
    p = computeDerivedDesign(cfg);
    in(k) = applyParamsToSimInput(Simulink.SimulationInput(modelName), p);
    in(k) = in(k).setModelParameter('StopTime', '4.0');
end
out = sim(in);
wheelCheckPass = false(numel(wheelChecks), 1);
for k = 1:numel(wheelChecks)
    th = out(k).logsout.get('theta_deg').Values;
    wheelCheckPass(k) = abs(th.Data(end)) < 1.0;
end

%% 5. Sweep 4: Square Face Size (100-200 mm) & Driver Drop (L298N vs MOSFET)
sideVec = linspace(0.100, 0.200, 35);
thL298N = zeros(size(sideVec));
thMosfet = zeros(size(sideVec));
tauFall = zeros(size(sideVec));
for k = 1:numel(sideVec)
    cfgL = baseCfg;
    cfgL.square_side_m = sideVec(k);
    cfgL.V_drop_driver_V = 2.0;
    pL = computeDerivedDesign(cfgL);
    thL298N(k) = pL.theta_max_recoverable_deg;
    tauFall(k) = pL.tau_fall_s * 1e3;

    cfgM = cfgL;
    cfgM.V_drop_driver_V = 0.2;
    pM = computeDerivedDesign(cfgM);
    thMosfet(k) = pM.theta_max_recoverable_deg;
end

% Nonlinear Simscape max recoverable angle check at 100, 140, 180 mm (L298N)
sideCheck = [0.100, 0.140, 0.180];
simMaxAngleL298N = zeros(size(sideCheck));
for idx = 1:numel(sideCheck)
    testAngles = 6:2:24;
    in = repmat(Simulink.SimulationInput(modelName), numel(testAngles), 1);
    for a = 1:numel(testAngles)
        cfg = baseCfg;
        cfg.square_side_m = sideCheck(idx);
        cfg.V_drop_driver_V = 2.0;
        cfg.theta0_deg = testAngles(a);
        cfg.dist_torque_Nm = 0.0;
        cfg.com_imbalance_offset_m = 0.0;
        p = computeDerivedDesign(cfg);
        in(a) = applyParamsToSimInput(Simulink.SimulationInput(modelName), p);
        in(a) = in(a).setModelParameter('StopTime', '3.5');
    end
    out = sim(in);
    maxOk = 0;
    for a = 1:numel(testAngles)
        th = out(a).logsout.get('theta_deg').Values;
        if abs(th.Data(end)) < 1.5
            maxOk = testAngles(a);
        end
    end
    simMaxAngleL298N(idx) = maxOk;
end

fprintf('\n=== Square Face Size Feasibility (90 mm Brass Wheel, JGB37-520 6.25:1, L298N) ===\n');
for idx = 1:numel(sideCheck)
    fprintf('Square = %3.0f mm -> Nonlinear Simscape max recoverable tilt >= %2d deg\n', ...
        sideCheck(idx)*1e3, simMaxAngleL298N(idx));
end

%% 6. Plot 2x2 Parametric Feasibility Trade-Study Figure
fig2 = figure('Visible', 'off', 'Position', [80, 80, 1280, 900], 'Color', 'w');
tl2 = tiledlayout(2, 2, 'TileSpacing', 'compact', 'Padding', 'compact');
title(tl2, '1-Wheel Balance Cube Parametric Physical Feasibility & Trade Studies', ...
    'FontWeight', 'bold', 'FontSize', 13);

% Panel 1: Nonlinear Simscape Motor Comparison
axP1 = nexttile;
hold(axP1, 'on'); grid(axP1, 'on');
for k = 1:nMot
    plot(axP1, simMotorResults{k}.time, simMotorResults{k}.theta, ...
        'Color', motors(k).color, 'LineWidth', 1.8, 'DisplayName', motors(k).name);
end
ylim(axP1, [-2, 12]);
xlabel(axP1, 'Time [s]');
ylabel(axP1, 'Tilt Angle \theta [deg]');
title(axP1, 'A. Nonlinear Simscape Motor Comparison (\theta_0 = 6^\circ)');
legend(axP1, 'Location', 'northeast', 'FontSize', 8);

% Panel 2: Motor Torque vs No-Load RPM Contour
axP2 = nexttile;
hold(axP2, 'on'); grid(axP2, 'on');
contourf(axP2, TGrid, RGrid, thMaxMotGrid, 0:2:30, 'LineColor', 'none');
colorbar(axP2);
[C2, h2] = contour(axP2, TGrid, RGrid, thMaxMotGrid, [5, 10, 15, 20], 'w--', 'LineWidth', 1.1);
clabel(C2, h2, 'Color', 'w', 'FontSize', 8);
contour(axP2, TGrid, RGrid, iStallGrid, [3.0, 3.0], 'r-', 'LineWidth', 2.0);
text(axP2, 0.25, 3400, '3 A Fuse Boundary (I_{stall} > 3 A above)', 'Color', [0.95 0.15 0.15], ...
    'FontWeight', 'bold', 'FontSize', 8.5, 'BackgroundColor', [1 1 1 0.75]);
for k = 1:nMot
    if motors(k).rpm <= 5000
        plot(axP2, motors(k).Tstall, motors(k).rpm, 'o', 'MarkerSize', 7, ...
            'MarkerFaceColor', motors(k).color, 'MarkerEdgeColor', 'w', 'LineWidth', 1.2);
        xTxt = motors(k).Tstall + 0.015;
        yTxt = motors(k).rpm + 130;
        if motors(k).Tstall > 0.65
            xTxt = motors(k).Tstall - 0.16;
            yTxt = motors(k).rpm + 160;
        end
        text(axP2, xTxt, yTxt, ...
            regexprep(motors(k).name, ' \(.*\)', ''), 'Color', 'w', 'FontSize', 7.5, 'FontWeight', 'bold');
    end
end
xlabel(axP2, 'Motor Rated Stall Torque @ 12 V [N\cdotm]');
ylabel(axP2, 'Motor No-Load Speed [RPM]');
title(axP2, 'B. Recoverable Tilt \theta_{max} [deg] vs. Motor Torque & Speed (L298N)');

% Panel 3: Flywheel Radius vs Rim Thickness
axP3 = nexttile;
hold(axP3, 'on'); grid(axP3, 'on');
contourf(axP3, RadGrid*1e3, RimGrid*1e3, thMaxWheelGrid, 8:0.5:22, 'LineColor', 'none');
clim(axP3, [10, 20]);
cb3 = colorbar(axP3);
ylabel(cb3, '\theta_{max} [deg]');
[C3, h3] = contour(axP3, RadGrid*1e3, RimGrid*1e3, thMaxWheelGrid, 10:2:20, 'w--', 'LineWidth', 1.0);
clabel(C3, h3, 'Color', 'w', 'FontSize', 8, 'LabelSpacing', 250);
[Cm, hm] = contour(axP3, RadGrid*1e3, RimGrid*1e3, massWheelGrid, [150, 250, 400, 600, 800], 'k:', 'LineWidth', 1.3);
clabel(Cm, hm, 'Color', 'k', 'FontSize', 8, 'LabelSpacing', 300);
for k = 1:numel(wheelChecks)
    if wheelCheckPass(k)
        plot(axP3, wheelChecks(k).r_m*1e3, wheelChecks(k).t_m*1e3, 'go', ...
            'MarkerSize', 8, 'MarkerFaceColor', [0.1 0.8 0.2], 'MarkerEdgeColor', 'k', 'LineWidth', 1.2);
    else
        plot(axP3, wheelChecks(k).r_m*1e3, wheelChecks(k).t_m*1e3, 'rx', ...
            'MarkerSize', 9, 'LineWidth', 2.0);
    end
end
xlabel(axP3, 'Brass Flywheel Outer Radius [mm]');
ylabel(axP3, 'Outer Rim Thickness [mm]');
title(axP3, 'C. Recoverable \theta_{max} [deg] & Wheel Mass [g] (Green/Red = 12^\circ Sim)');

% Panel 4: Square Size & Driver Trade-off
axP4 = nexttile;
hold(axP4, 'on'); grid(axP4, 'on');
yyaxis(axP4, 'left');
plot(axP4, sideVec*1e3, thL298N, '-', 'Color', [0.05 0.35 0.72], 'LineWidth', 1.8, ...
    'DisplayName', 'L298N (10.0 V eff) Linear Bound');
plot(axP4, sideVec*1e3, thMosfet, '--', 'Color', [0.10 0.58 0.32], 'LineWidth', 1.8, ...
    'DisplayName', 'MOSFET (11.8 V eff) Linear Bound');
plot(axP4, sideCheck*1e3, simMaxAngleL298N, 's', 'Color', [0.05 0.35 0.72], ...
    'MarkerFaceColor', [0.05 0.35 0.72], 'MarkerSize', 7, 'DisplayName', 'L298N Nonlinear Sim Max');
ylabel(axP4, 'Max Recoverable Tilt \theta_{max} [deg]');
ylim(axP4, [0, 30]);

yyaxis(axP4, 'right');
plot(axP4, sideVec*1e3, tauFall, '-.', 'Color', [0.82 0.35 0.08], 'LineWidth', 1.6, ...
    'DisplayName', 'Fall Time Constant \tau_{fall} [ms]');
ylabel(axP4, 'Open-Loop Fall Time \tau_{fall} [ms]');
ylim(axP4, [60, 140]);

xlabel(axP4, 'Square Face Side Length [mm]');
title(axP4, 'D. Square Size vs. Authority & Driver Drop');
legend(axP4, 'Location', 'northeast', 'FontSize', 8);

outPlot2 = fullfile(resultsDir, 'parametric_feasibility_sweeps.png');
exportgraphics(fig2, outPlot2, 'Resolution', 200);
close(fig2);

fprintf('\nSaved plots:\n  %s\n  %s\n', outPlot1, outPlot2);

% Restore clean baseline parameters in base workspace
clear quiet_params;
quiet_params = true;
params_1d_cube;

%% Local Helper Functions
function cfg = defaultDesignConfig()
    cfg = struct( ...
        'square_side_m', 0.140, ...
        'plate_thickness_m', 0.006, ...
        'plate_density_kgm3', 1190, ...
        'pivot_corner_offset_m', 0.015, ...
        'extra_frame_mass_kg', 0.045, ...
        'com_imbalance_offset_m', 0.0005, ...
        'b_pivot_Nms', 5.0e-4, ...
        'flywheel_density_kgm3', 8500, ...
        'flywheel_outer_radius_m', 0.045, ...
        'flywheel_inner_radius_m', 0.033, ...
        'flywheel_rim_thickness_m', 0.012, ...
        'flywheel_web_thickness_m', 0.003, ...
        'motor_mass_kg', 0.155, ...
        'motor_rotor_mass_kg', 0.020, ...
        'motor_rotor_J_kgm2', 5.0e-6, ...
        'motor_V_rated_V', 12.0, ...
        'motor_T_stall_Nm', 0.24, ...
        'motor_rpm_noload', 1600, ...
        'motor_La_H', 0.5e-3, ...
        'motor_b_viscous_Nms', 1.5e-5, ...
        'V_supply_V', 12.0, ...
        'V_drop_driver_V', 2.0, ...
        'I_fuse_limit_A', 3.0, ...
        'Ts_ctrl', 0.005, ...
        'theta0_deg', 5.0, ...
        'catch_limit_deg', 30.0, ...
        'dist_torque_Nm', 0.04, ...
        'dist_start_s', 2.5, ...
        'dist_duration_s', 0.1);
end

function p = computeDerivedDesign(cfg)
    g_acc = 9.80665;
    p = cfg;
    p.dist_end_s = cfg.dist_start_s + cfg.dist_duration_s;
    p.L_center_m = cfg.square_side_m / sqrt(2) - cfg.pivot_corner_offset_m;
    p.plate_mass_kg = cfg.plate_density_kgm3 * (cfg.square_side_m^2) * cfg.plate_thickness_m + cfg.extra_frame_mass_kg;
    I_plate_cm = (1/6) * p.plate_mass_kg * (cfg.square_side_m^2);

    p.motor_stator_mass_kg = max(0.01, cfg.motor_mass_kg - cfg.motor_rotor_mass_kg);
    I_stator_cm = 0.5 * p.motor_stator_mass_kg * (0.0185^2);

    r_in = min(cfg.flywheel_inner_radius_m, 0.95 * cfg.flywheel_outer_radius_m);
    t_web = min(cfg.flywheel_web_thickness_m, cfg.flywheel_rim_thickness_m);
    m_web = cfg.flywheel_density_kgm3 * pi * (r_in^2) * t_web;
    m_rim = cfg.flywheel_density_kgm3 * pi * (cfg.flywheel_outer_radius_m^2 - r_in^2) * cfg.flywheel_rim_thickness_m;
    p.flywheel_mass_kg = m_web + m_rim;

    I_fw = 0.5 * m_web * (r_in^2) + 0.5 * m_rim * (cfg.flywheel_outer_radius_m^2 + r_in^2);
    p.wheel_total_mass_kg = p.flywheel_mass_kg + cfg.motor_rotor_mass_kg;
    p.I_wheel_spin_kgm2 = I_fw + cfg.motor_rotor_J_kgm2;
    p.I_wheel_trans_kgm2 = 0.5 * p.I_wheel_spin_kgm2 + (1/12) * p.wheel_total_mass_kg * (cfg.flywheel_rim_thickness_m^2);

    p.total_mass_kg = p.plate_mass_kg + p.motor_stator_mass_kg + p.wheel_total_mass_kg;
    p.I_pendulum_reduced_kgm2 = I_plate_cm + I_stator_cm + p.total_mass_kg * (p.L_center_m^2 + cfg.com_imbalance_offset_m^2);
    p.Mgl_eff_Nm = p.total_mass_kg * g_acc * p.L_center_m;
    p.p_unstable_rads = sqrt(p.Mgl_eff_Nm / p.I_pendulum_reduced_kgm2);
    p.tau_fall_s = 1.0 / p.p_unstable_rads;

    p.V_max_eff_V = max(1.0, cfg.V_supply_V - cfg.V_drop_driver_V);
    w_noload_rads = cfg.motor_rpm_noload * (2 * pi / 60);
    p.motor_Kt_NmA = cfg.motor_V_rated_V / w_noload_rads;
    p.motor_Ra_Ohm = (cfg.motor_V_rated_V * p.motor_Kt_NmA) / cfg.motor_T_stall_Nm;
    p.w_max_eff_rads = p.V_max_eff_V / p.motor_Kt_NmA;
    p.w_max_eff_rpm = p.w_max_eff_rads * (60 / (2 * pi));
    p.I_stall_eff_A = p.V_max_eff_V / p.motor_Ra_Ohm;

    ke_damp = (p.motor_Kt_NmA^2) / p.motor_Ra_Ohm + cfg.motor_b_viscous_Nms;
    km_gain = p.motor_Kt_NmA / p.motor_Ra_Ohm;
    Ip = p.I_pendulum_reduced_kgm2;
    Iw = p.I_wheel_spin_kgm2;

    A_3 = [0, 1, 0;
           p.Mgl_eff_Nm/Ip, -cfg.b_pivot_Nms/Ip, ke_damp/Ip;
          -p.Mgl_eff_Nm/Ip,  cfg.b_pivot_Nms/Ip, -(1/Iw + 1/Ip)*ke_damp];
    B_3 = [0; -km_gain/Ip; (1/Iw + 1/Ip)*km_gain];

    [V_left, D_eig] = eig(A_3.');
    [lambda_u, idx_u] = max(real(diag(D_eig)));
    v_u = real(V_left(:, idx_u));
    p.theta_max_recoverable_deg = rad2deg(min(pi/2, abs(v_u.' * B_3) * p.V_max_eff_V / (lambda_u * abs(v_u(1)))));

    Ac = [A_3, zeros(3, 1); 0, 0, 1, 0];
    Bc = [B_3; 0];
    sys_d = c2d(ss(Ac, Bc, eye(4), zeros(4, 1)), cfg.Ts_ctrl, 'zoh');
    Q_lqr = diag([250, 12, 0.04, 0.015]);
    R_lqr = 0.6;
    p.K_lqr = dlqr(sys_d.A, sys_d.B, Q_lqr, R_lqr);
    p.catch_limit_rad = deg2rad(cfg.catch_limit_deg);
    p.int_w_max = 25.0;
end

function in = applyParamsToSimInput(in, p)
    vars = {'theta0_deg', 'b_pivot_Nms', 'com_imbalance_offset_m', 'L_center_m', ...
            'square_side_m', 'plate_thickness_m', 'plate_mass_kg', ...
            'motor_stator_mass_kg', 'flywheel_outer_radius_m', 'flywheel_rim_thickness_m', ...
            'wheel_total_mass_kg', 'I_wheel_trans_kgm2', 'I_wheel_spin_kgm2', ...
            'motor_Ra_Ohm', 'motor_La_H', 'motor_Kt_NmA', 'motor_b_viscous_Nms', ...
            'Ts_ctrl', 'int_w_max', 'K_lqr', 'catch_limit_rad', 'V_max_eff_V', ...
            'dist_start_s', 'dist_end_s', 'dist_torque_Nm'};
    for k = 1:numel(vars)
        in = in.setVariable(vars{k}, p.(vars{k}));
    end
end
