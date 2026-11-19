%% evaluate_flywheel_variants.m
% Is a given flywheel "enough"?  Compares the CAD-derived mass properties of
% the rig variants (acrylic + brass, all-printed + bolt ballast) using the
% same motor, driver and LQR weights as params_1d_cube.m:
%
%   * linear metrics: unstable pole, static torque angle, recoverable tilt
%   * re-synthesised 4-state dLQR per variant, and the shipped K on each plant
%   * nonlinear closed-loop simulation (sin(theta) gravity, voltage saturation,
%     back-EMF limited wheel speed, ZOH controller at Ts_ctrl) of the standard
%     start (theta0_deg) + tap disturbance, and the largest initial tilt that
%     is still recovered.
%
% Inputs: cad/cad_verification_report.json, cad/printed_verification_report.json
% (written by the FreeCAD generators).

here = fileparts(mfilename('fullpath'));
quiet_params = true;
run(fullfile(here, 'params_1d_cube.m'));
K_shipped = K_lqr;

files = {'acrylic_brass', fullfile(here, '..', 'cad', 'cad_verification_report.json');
         'all_printed',   fullfile(here, '..', 'cad', 'printed_verification_report.json')};

motor = struct('km', km_gain, 'ke', ke_damp, 'Vmax', V_max_eff_V, 'b', b_pivot_Nms, ...
               'Kt', motor_Kt_NmA, 'Ra', motor_Ra_Ohm, 'Imax', I_fuse_limit_A);
sim = struct('Ts', Ts_ctrl, 'T', 6.0, 'theta0', deg2rad(theta0_deg), 'tau_d', dist_torque_Nm, ...
             't_d0', dist_start_s, 't_d1', dist_end_s, 'catch', deg2rad(catch_limit_deg));

% Baseline straight from params_1d_cube.m for reference
V = struct('name', {}, 'Ip', {}, 'Iw', {}, 'Mgl', {}, 'm_wheel', {}, 'm_total', {});
V(end+1) = struct('name', 'params_1d_cube', 'Ip', Ip, 'Iw', Iw, 'Mgl', Mgl_eff_Nm, ...
                  'm_wheel', flywheel_mass_kg, 'm_total', total_mass_kg);
for i = 1:size(files, 1)
    if ~isfile(files{i, 2}), warning('missing %s', files{i, 2}); continue; end
    r = jsondecode(fileread(files{i, 2})).mass_inertia_comparison;
    V(end+1) = struct('name', files{i, 1}, 'Ip', r.cad_Ip_reduced_kgm2, ...
        'Iw', r.cad_flywheel_I_spin_exact_kgm2 + motor_rotor_J_kgm2, 'Mgl', r.cad_Mgl_Nm, ...
        'm_wheel', r.cad_flywheel_kg, 'm_total', r.cad_total_rotating_kg); %#ok<SAGROW>
end

fprintf('\n%-16s %9s %9s %9s %8s %8s %8s %9s %9s\n', 'variant', 'm_wheel', 'Iw', 'Ip', 'Mgl', 'p[r/s]', 'stat[deg]', 'lin_rec', 'rho(Kship)');
res = struct();
for i = 1:numel(V)
    v = V(i);
    [A3, B3] = plant3(v.Ip, v.Iw, v.Mgl, motor);
    Ac = [A3, zeros(3,1); 0 0 1 0]; Bc = [B3; 0];
    sysd = c2d(ss(Ac, Bc, eye(4), zeros(4,1)), Ts_ctrl, 'zoh');
    K = dlqr(sysd.A, sysd.B, Q_lqr, R_lqr);
    rho_ship = max(abs(eig(sysd.A - sysd.B * K_shipped)));
    [VL, D] = eig(A3.'); [lu, iu] = max(real(diag(D))); vu = real(VL(:, iu));
    lin_rec = rad2deg(min(pi/2, abs(vu.' * B3) * motor.Vmax / (lu * abs(vu(1)))));
    stat = asind(min(1, motor.km * motor.Vmax / v.Mgl));
    fprintf('%-16s %8.3fkg %9.3e %9.3e %8.4f %8.2f %8.2f %9.2f %9.4f\n', v.name, v.m_wheel, v.Iw, v.Ip, v.Mgl, ...
        sqrt(v.Mgl / v.Ip), stat, lin_rec, rho_ship);

    out = simulate(v, K, motor, sim, sim.theta0, true);
    th_max = bisect_recoverable(v, K, motor, sim);
    res.(v.name) = struct('K', K, 'rho_shipped', rho_ship, 'lin_rec_deg', lin_rec, 'static_deg', stat, ...
                          'sim', out, 'nonlin_rec_deg', th_max);
end

fprintf('\nNonlinear closed loop (own dLQR, |V| <= %.1f V): %.0f deg start + %.3f N*m x %.1f s tap\n', ...
    motor.Vmax, theta0_deg, dist_torque_Nm, dist_duration_s);
fprintf('%-16s %10s %12s %12s %10s %12s %14s\n', 'variant', 'upright?', 'peak tilt', 'peak wheel', 'peak |V|', 'peak |I|', 'max start tilt');
w_noload = motor.Vmax / motor.Kt * 60 / (2*pi);
for i = 1:numel(V)
    o = res.(V(i).name).sim;
    fprintf('%-16s %10s %10.2f deg %8.0f rpm %8.2f V %10.2f A %12.1f deg\n', V(i).name, yesno(o.ok), ...
        rad2deg(o.th_peak), o.w_peak * 60 / (2*pi), o.V_peak, o.I_peak, res.(V(i).name).nonlin_rec_deg);
end
fprintf('(back-EMF speed limit at %.1f V: %.0f rpm; fuse %.1f A)\n', motor.Vmax, w_noload, motor.Imax);

function [A3, B3] = plant3(Ip, Iw, Mgl, m)
    A3 = [0, 1, 0; Mgl/Ip, -m.b/Ip, m.ke/Ip; -Mgl/Ip, m.b/Ip, -(1/Iw + 1/Ip)*m.ke];
    B3 = [0; -m.km/Ip; (1/Iw + 1/Ip)*m.km];
end

function out = simulate(v, K, m, s, theta0, with_tap)
    x = [theta0; 0; 0]; xi = 0;               % theta, theta_dot, wheel speed rel. frame; integral of wheel speed
    n = round(s.T / s.Ts);
    th_peak = 0; w_peak = 0; V_peak = 0; I_peak = 0; ok = true;
    opts = odeset('RelTol', 1e-6, 'AbsTol', 1e-8);
    for k = 0:n-1
        t0 = k * s.Ts;
        u = -K * [x; xi];
        Vc = max(-m.Vmax, min(m.Vmax, u));
        tau = @(t) with_tap * s.tau_d * (t >= s.t_d0 && t < s.t_d1);
        f = @(t, z) dyn(z, Vc, tau(t), v, m);
        [~, Z] = ode45(f, [t0, t0 + s.Ts], [x; xi], opts);
        x = Z(end, 1:3).'; xi = Z(end, 4);
        th_peak = max(th_peak, abs(x(1))); w_peak = max(w_peak, abs(x(3)));
        V_peak = max(V_peak, abs(Vc)); I_peak = max(I_peak, abs((Vc - m.Kt * x(3)) / m.Ra));
        if abs(x(1)) > s.catch, ok = false; break; end
    end
    ok = ok && abs(x(1)) < deg2rad(1.0);
    out = struct('ok', ok, 'th_peak', th_peak, 'w_peak', w_peak, 'V_peak', V_peak, 'I_peak', I_peak);
end

function dz = dyn(z, Vc, tau_d, v, m)
    th = z(1); thd = z(2); w = z(3);
    T = m.km * Vc - m.ke * w;                  % motor torque on the wheel (reaction on the frame)
    thdd = (v.Mgl * sin(th) - m.b * thd - T + tau_d) / v.Ip;
    wd = T / v.Iw - thdd;
    dz = [thd; thdd; wd; w];
end

function th_max = bisect_recoverable(v, K, m, s)
    lo = 0; hi = 40;
    for it = 1:12
        mid = 0.5 * (lo + hi);
        o = simulate(v, K, m, s, deg2rad(mid), false);
        if o.ok, lo = mid; else, hi = mid; end
    end
    th_max = lo;
end

function s = yesno(b)
    if b, s = 'yes'; else, s = 'NO'; end
end
