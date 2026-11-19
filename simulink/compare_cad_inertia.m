%% compare_cad_inertia.m
% Term-by-term comparison of the pendulum mass properties assumed in
% params_1d_cube.m against the solid-model values that cad/generate_cad_models.py
% writes to cad/cad_verification_report.json, and a robustness check of the
% shipped LQR gains on the CAD-derived plant.
%
% Usage:  run from the simulink/ folder (or anywhere: paths are resolved
%         relative to this file).  Optional: set report_json before calling.

here = fileparts(mfilename('fullpath'));
if ~exist('report_json', 'var')
    report_json = fullfile(here, '..', 'cad', 'cad_verification_report.json');
end
cad = jsondecode(fileread(report_json));
mi = cad.mass_inertia_comparison;
bd = mi.breakdown;

%% 1. Baseline: params_1d_cube.m exactly as shipped
quiet_params = true;
run(fullfile(here, 'params_1d_cube.m'));
L = L_center_m;
base.Ip = Ip; base.Iw = Iw; base.Mgl = Mgl_eff_Nm; base.K = K_lqr;
base.p = p_unstable_rads; base.theta_rec = theta_max_recoverable_deg;

%% 2. Term-by-term breakdown about the pivot (wheel spin inertia excluded)
plate_only_kg = plate_density_kgm3 * square_side_m^2 * plate_thickness_m;
P = struct();   % params_1d_cube.m terms
P.plate      = (1/6) * plate_only_kg * square_side_m^2 + plate_only_kg * L^2;
P.extra_45g  = (1/6) * extra_frame_mass_kg * square_side_m^2 + extra_frame_mass_kg * L^2;
P.motor      = I_stator_cm_kgm2 + motor_stator_mass_kg * L^2 + motor_rotor_mass_kg * L^2;
P.flywheel   = flywheel_mass_kg * L^2;
P.imbalance  = total_mass_kg * com_imbalance_offset_m^2;

C = struct();   % CAD terms (same grouping)
C.plate      = bd.plate.I_pivot_kgm2;
C.extra_45g  = bd.clamp_hub.I_pivot_kgm2 + bd.imu.I_pivot_kgm2 + bd.fasteners.I_pivot_kgm2 + bd.pivot_shaft.I_pivot_kgm2;
C.motor      = bd.motor_encoder.I_pivot_kgm2;
C.flywheel   = bd.flywheel_point_mass.I_pivot_kgm2;
C.imbalance  = 0;

terms = fieldnames(P);
fprintf('\n=== Pendulum inertia about the pivot, Ip [kg*m^2] ===\n');
fprintf('%-12s %12s %12s %12s   %s\n', 'term', 'params', 'CAD', 'delta', 'why');
why = struct( ...
    'plate',     'full 140x140 square vs cut-out plate (holes, nose)', ...
    'extra_45g', 'params spreads 45 g over the square at L; CAD: clamp hub ~3 mm, fasteners ~17 mm, IMU 141 mm', ...
    'motor',     'params puts stator+rotor at L=84 mm; CAD can/rotor axis is 7 mm lower (eccentric gearbox)', ...
    'flywheel',  'params rim+web formula omits the 16 mm hub boss', ...
    'imbalance', '0.5 mm CoM offset term (negligible)');
sumP = 0; sumC = 0;
for k = 1:numel(terms)
    t = terms{k};
    fprintf('%-12s %12.4e %12.4e %+12.4e   %s\n', t, P.(t), C.(t), C.(t) - P.(t), why.(t));
    sumP = sumP + P.(t); sumC = sumC + C.(t);
end
fprintf('%-12s %12.4e %12.4e %+12.4e   (params Ip as computed: %.4e)\n', 'TOTAL', sumP, sumC, sumC - sumP, base.Ip);

%% 3. Gravity torque and spin inertia
mgl_cad = mi.cad_Mgl_Nm;
iw_cad = mi.cad_flywheel_I_spin_exact_kgm2 + motor_rotor_J_kgm2;
fprintf('\n=== Gravity stiffness & wheel ===\n');
fprintf('Mgl  params %.4f N*m | CAD %.4f N*m (%+.1f %%)\n', base.Mgl, mgl_cad, 100*(mgl_cad/base.Mgl - 1));
fprintf('Iw   params %.4e     | CAD %.4e     (%+.1f %%)\n', base.Iw, iw_cad, 100*(iw_cad/base.Iw - 1));
fprintf('m_total params %.4f kg | CAD %.4f kg (CAD adds the 38 g shaft on the pivot axis: no torque, ~no inertia)\n', ...
    total_mass_kg, mi.cad_total_rotating_kg);

%% 4. Plant with CAD mass properties
plantFor = @(Ip_, Iw_, Mgl_) deal( ...
    [0, 1, 0; Mgl_/Ip_, -b_pivot_Nms/Ip_, ke_damp/Ip_; -Mgl_/Ip_, b_pivot_Nms/Ip_, -(1/Iw_ + 1/Ip_)*ke_damp], ...
    [0; -km_gain/Ip_; (1/Iw_ + 1/Ip_)*km_gain]);
[A3c, B3c] = plantFor(sumC, iw_cad, mgl_cad);
p_cad = sqrt(mgl_cad / sumC);
Acc = [A3c, zeros(3,1); 0 0 1 0]; Bcc = [B3c; 0];
sysc = c2d(ss(Acc, Bcc, eye(4), zeros(4,1)), Ts_ctrl, 'zoh');
rho_shipped_on_cad = max(abs(eig(sysc.A - sysc.B * base.K)));
K_cad = dlqr(sysc.A, sysc.B, Q_lqr, R_lqr);

[V_left, D_eig] = eig(A3c.'); [lu, iu] = max(real(diag(D_eig))); vu = real(V_left(:, iu));
theta_rec_cad = rad2deg(min(pi/2, abs(vu.' * B3c) * V_max_eff_V / (lu * abs(vu(1)))));

fprintf('\n=== Effect on control ===\n');
fprintf('Unstable pole     params %.2f rad/s | CAD %.2f rad/s (%+.1f %%)\n', base.p, p_cad, 100*(p_cad/base.p - 1));
fprintf('Recoverable tilt  params %.2f deg   | CAD %.2f deg\n', base.theta_rec, theta_rec_cad);
fprintf('Shipped K on CAD plant: closed-loop max |eig| = %.4f (%s)\n', rho_shipped_on_cad, ...
    ternary(rho_shipped_on_cad < 1, 'STABLE', 'UNSTABLE'));
fprintf('K shipped [%.3f %.3f %.4f %.4f]\nK CAD     [%.3f %.3f %.4f %.4f]\n', base.K, K_cad);

cmp = struct('Ip_params', base.Ip, 'Ip_cad', sumC, 'Mgl_params', base.Mgl, 'Mgl_cad', mgl_cad, ...
    'Iw_params', base.Iw, 'Iw_cad', iw_cad, 'p_params', base.p, 'p_cad', p_cad, ...
    'rho_shipped_on_cad', rho_shipped_on_cad, 'K_shipped', base.K, 'K_cad', K_cad);

function out = ternary(c, a, b)
    if c, out = a; else, out = b; end
end
