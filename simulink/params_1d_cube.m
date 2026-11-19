%% params_1d_cube.m
% Parametric configuration, physical feasibility calculations, and discrete
% 4-state LQR controller synthesis for the 1-reaction-wheel Balance Cube
% (1D Cubli proof-of-concept).
%
% Any primary design variable can be overridden in the workspace before
% calling this script to evaluate parametric sweeps or hardware trade-offs.

%% 1. Primary Parametric Design Inputs (Override-Safe)
% Square Face & Mechanical Frame (matches acrylic diamond + pillow block)
if ~exist('square_side_m', 'var'),        square_side_m = 0.140; end         % Square face side length [m]
if ~exist('plate_thickness_m', 'var'),    plate_thickness_m = 0.006; end     % Plate thickness [m]
if ~exist('plate_density_kgm3', 'var'),   plate_density_kgm3 = 1190; end     % Cast acrylic PMMA [kg/m^3]
if ~exist('pivot_corner_offset_m', 'var'),pivot_corner_offset_m = 0.015; end % Corner tip to bearing axis along diagonal [m]
if ~exist('extra_frame_mass_kg', 'var'),  extra_frame_mass_kg = 0.045; end   % Brackets, shaft collar, fasteners, wires [kg]
if ~exist('com_imbalance_offset_m', 'var'), com_imbalance_offset_m = 0.0005; end % Lateral CoM offset (+X) [m]
if ~exist('b_pivot_Nms', 'var'),          b_pivot_Nms = 5.0e-4; end          % Pillow-block viscous damping [N*m/(rad/s)]

% Machined Rimmed Brass Flywheel
if ~exist('flywheel_density_kgm3', 'var'), flywheel_density_kgm3 = 8500; end     % C36000 Brass [kg/m^3]
if ~exist('flywheel_outer_radius_m', 'var'), flywheel_outer_radius_m = 0.045; end % 90 mm OD [m]
if ~exist('flywheel_inner_radius_m', 'var'), flywheel_inner_radius_m = 0.033; end % 66 mm web recess radius [m]
if ~exist('flywheel_rim_thickness_m', 'var'), flywheel_rim_thickness_m = 0.012; end % 12 mm outer rim width [m]
if ~exist('flywheel_web_thickness_m', 'var'), flywheel_web_thickness_m = 0.003; end % 3 mm central web thickness [m]

% Brushed DC Motor + Rear Encoder (Baseline: 12V JGB37-520 6.25:1 class)
if ~exist('motor_mass_kg', 'var'),        motor_mass_kg = 0.155; end         % Total motor + encoder mass [kg]
if ~exist('motor_rotor_mass_kg', 'var'),  motor_rotor_mass_kg = 0.020; end   % Effective rotor + shaft mass [kg]
if ~exist('motor_rotor_J_kgm2', 'var'),   motor_rotor_J_kgm2 = 5.0e-6; end   % Reflected motor rotor inertia [kg*m^2]
if ~exist('motor_V_rated_V', 'var'),      motor_V_rated_V = 12.0; end        % Motor rated voltage [V]
if ~exist('motor_T_stall_Nm', 'var'),     motor_T_stall_Nm = 0.24; end       % Rated stall torque at V_rated [N*m]
if ~exist('motor_rpm_noload', 'var'),     motor_rpm_noload = 1600; end       % Rated no-load speed at V_rated [RPM]
if ~exist('motor_La_H', 'var'),           motor_La_H = 0.5e-3; end           % Armature inductance [H]
if ~exist('motor_b_viscous_Nms', 'var'),  motor_b_viscous_Nms = 1.5e-5; end  % Motor internal viscous damping [N*m/(rad/s)]

% Power Supply & H-Bridge Driver (From Balance_cube.kicad_sch)
if ~exist('V_supply_V', 'var'),           V_supply_V = 12.0; end             % +12V supply rail [V]
if ~exist('V_drop_driver_V', 'var'),      V_drop_driver_V = 2.0; end         % L298N Darlington drop (~2.0V; ~0.2V MOSFET) [V]
if ~exist('I_fuse_limit_A', 'var'),       I_fuse_limit_A = 3.0; end          % KiCad Polyfuse rating [A]

% Realistic Mechanical Non-Idealities (for BalanceCube_1D_Realistic.slx)
if ~exist('gear_backlash_deg', 'var'),    gear_backlash_deg = 1.0; end         % Total gearbox free-play [deg]
if ~exist('motor_T_breakaway_Nm', 'var'), motor_T_breakaway_Nm = 0.010; end  % Brush/gear breakaway stiction [N*m]
if ~exist('motor_T_coulomb_Nm', 'var'),   motor_T_coulomb_Nm = 0.008; end    % Running Coulomb friction [N*m]
if ~exist('table_stop_deg', 'var'),       table_stop_deg = 45.0; end         % Physical table rest stop [+-deg]

% Simulation, Control Rate & Disturbance Parameters
if ~exist('Ts_ctrl', 'var'),              Ts_ctrl = 0.005; end               % 200 Hz discrete MCU loop rate [s]
if ~exist('theta0_deg', 'var'),           theta0_deg = 5.0; end              % Initial tilt angle from upright [deg]
if ~exist('catch_limit_deg', 'var'),      catch_limit_deg = 30.0; end        % Fall cutoff threshold [deg]
if ~exist('dist_torque_Nm', 'var'),       dist_torque_Nm = 0.04; end         % External tap disturbance [N*m]
if ~exist('dist_start_s', 'var'),         dist_start_s = 2.5; end            % Tap disturbance start time [s]
if ~exist('dist_duration_s', 'var'),      dist_duration_s = 0.1; end         % Tap disturbance pulse width [s]

%% 2. Derived Geometry, Mass & Inertias
g_acc = 9.80665;
dist_end_s = dist_start_s + dist_duration_s;

% Distance from pivot axis to square center along the upright diagonal
L_center_m = square_side_m / sqrt(2) - pivot_corner_offset_m;

% Square plate + mounting hardware mass and planar moment of inertia about CoM
plate_mass_kg = plate_density_kgm3 * (square_side_m^2) * plate_thickness_m + extra_frame_mass_kg;
I_plate_cm_kgm2 = (1/6) * plate_mass_kg * (square_side_m^2);

% Motor stator (mounted rigidly to square center)
motor_stator_mass_kg = max(0.01, motor_mass_kg - motor_rotor_mass_kg);
I_stator_cm_kgm2 = 0.5 * motor_stator_mass_kg * (0.0185^2);

% Machined rimmed flywheel mass and axial/transverse inertias
r_in_eff = min(flywheel_inner_radius_m, 0.95 * flywheel_outer_radius_m);
t_web_eff = min(flywheel_web_thickness_m, flywheel_rim_thickness_m);
m_web_kg = flywheel_density_kgm3 * pi * (r_in_eff^2) * t_web_eff;
m_rim_kg = flywheel_density_kgm3 * pi * (flywheel_outer_radius_m^2 - r_in_eff^2) * flywheel_rim_thickness_m;
flywheel_mass_kg = m_web_kg + m_rim_kg;

I_flywheel_kgm2 = 0.5 * m_web_kg * (r_in_eff^2) + ...
                  0.5 * m_rim_kg * (flywheel_outer_radius_m^2 + r_in_eff^2);

wheel_total_mass_kg = flywheel_mass_kg + motor_rotor_mass_kg;
I_wheel_spin_kgm2 = I_flywheel_kgm2 + motor_rotor_J_kgm2;
I_wheel_trans_kgm2 = 0.5 * I_wheel_spin_kgm2 + (1/12) * wheel_total_mass_kg * (flywheel_rim_thickness_m^2);

% Combined inverted pendulum reduced inertia about pivot (excluding wheel spin J_w)
total_mass_kg = plate_mass_kg + motor_stator_mass_kg + wheel_total_mass_kg;
I_pendulum_reduced_kgm2 = I_plate_cm_kgm2 + I_stator_cm_kgm2 + ...
                          total_mass_kg * (L_center_m^2 + com_imbalance_offset_m^2);
Mgl_eff_Nm = total_mass_kg * g_acc * L_center_m;

% Unstable open-loop natural frequency (rad/s) and fall time constant (s)
p_unstable_rads = sqrt(Mgl_eff_Nm / I_pendulum_reduced_kgm2);
tau_fall_s = 1.0 / p_unstable_rads;

%% 3. Brushed Motor Circuit & Electrical Limits
V_max_eff_V = max(1.0, V_supply_V - V_drop_driver_V);
w_noload_rads = motor_rpm_noload * (2 * pi / 60);
motor_Kt_NmA = motor_V_rated_V / w_noload_rads;                     % Ke = Kt in SI [V/(rad/s) = N*m/A]
motor_Ra_Ohm = (motor_V_rated_V * motor_Kt_NmA) / motor_T_stall_Nm;
w_max_eff_rads = V_max_eff_V / motor_Kt_NmA;
w_max_eff_rpm = w_max_eff_rads * (60 / (2 * pi));

T_max_voltage_Nm = motor_T_stall_Nm * (V_max_eff_V / motor_V_rated_V);
T_max_fuse_Nm = motor_Kt_NmA * I_fuse_limit_A;
T_max_eff_Nm = min(T_max_voltage_Nm, T_max_fuse_Nm);
I_stall_eff_A = V_max_eff_V / motor_Ra_Ohm;

%% 4. Linearized Plant Matrices & Analytical Feasibility Metrics
ke_damp = (motor_Kt_NmA^2) / motor_Ra_Ohm + motor_b_viscous_Nms;
km_gain = motor_Kt_NmA / motor_Ra_Ohm;
Ip = I_pendulum_reduced_kgm2;
Iw = I_wheel_spin_kgm2;

% 3-state core plant: z = [theta; theta_dot; omega_w], input u = V_cmd
A_3 = [0, 1, 0;
       Mgl_eff_Nm/Ip, -b_pivot_Nms/Ip, ke_damp/Ip;
      -Mgl_eff_Nm/Ip,  b_pivot_Nms/Ip, -(1/Iw + 1/Ip)*ke_damp];
B_3 = [0;
      -km_gain/Ip;
      (1/Iw + 1/Ip)*km_gain];

% Static torque-balance ceiling
theta_max_static_voltage_deg = asind(min(1.0, T_max_voltage_Nm / Mgl_eff_Nm));
theta_max_static_fuse_deg = asind(min(1.0, T_max_fuse_Nm / Mgl_eff_Nm));

% Exact bounded-input null-controllability limit from rest (theta0, 0, 0)
% Using the unstable left eigenvector v_u' * A_3 = lambda_u * v_u'
[V_left, D_eig] = eig(A_3.');
eig_vals = diag(D_eig);
[lambda_u, idx_u] = max(real(eig_vals));
v_u = real(V_left(:, idx_u));
theta_max_recoverable_rad = abs(v_u.' * B_3) * V_max_eff_V / (lambda_u * abs(v_u(1)));
theta_max_recoverable_deg = rad2deg(min(pi/2, theta_max_recoverable_rad));

% Angular momentum & 45-deg self-jump-up torque/energy assessment
H_max_Nms = I_wheel_spin_kgm2 * w_max_eff_rads;
T_lift_45deg_Nm = Mgl_eff_Nm * sind(45);
T_ebrake_max_Nm = 2.0 * T_max_voltage_Nm;  % Theoretical 2*V_max reverse-plugging torque (2*I_stall)
E_jump_needed_J = Mgl_eff_Nm * (1 - cosd(45));
% Maximum rotational KE transferable during an instantaneous mechanical brake from w_max
E_wheel_max_J = 0.5 * Iw * (w_max_eff_rads^2);
E_jump_transfer_J = E_wheel_max_J * (Iw / (Ip + Iw));
jump_up_energy_ratio = E_jump_transfer_J / E_jump_needed_J;

%% 5. Discrete 4-State LQR Controller Synthesis (Includes Wheel Speed Integral)
% State x = [theta; theta_dot; omega_w; int_omega_w]
Ac = [A_3, zeros(3, 1);
      0, 0, 1, 0];
Bc = [B_3;
      0];

sys_d = c2d(ss(Ac, Bc, eye(4), zeros(4, 1)), Ts_ctrl, 'zoh');

% Weight tilt & angular rate tightly; penalize wheel speed & its integral
% so steady-state wheel speed decays to 0 RPM even with CoM asymmetry.
Q_lqr = diag([250, 12, 0.04, 0.015]);
R_lqr = 0.6;
K_lqr = dlqr(sys_d.A, sys_d.B, Q_lqr, R_lqr);

catch_limit_rad = deg2rad(catch_limit_deg);
int_w_max = 25.0;

%% 6. Summary Printout (Suppressed when quiet_params == true)
if ~exist('quiet_params', 'var') || ~quiet_params
    fprintf('=== 1D Balance Cube Parametric Summary ===\n');
    fprintf('Square: %.0f mm | Plate: %.3f kg | Flywheel: %.3f kg | Motor: %.3f kg | Total: %.3f kg\n', ...
        square_side_m*1e3, plate_mass_kg, flywheel_mass_kg, motor_mass_kg, total_mass_kg);
    fprintf('CoM height L: %.1f mm | Ip: %.5f kg*m^2 | Iw: %.6f kg*m^2 (Iw/Ip = %.3f)\n', ...
        L_center_m*1e3, Ip, Iw, Iw/Ip);
    fprintf('Unstable pole p: %.2f rad/s (fall time tau: %.0f ms)\n', p_unstable_rads, tau_fall_s*1e3);
    fprintf('Motor Kt: %.4f N*m/A | Ra: %.3f Ohm | V_eff_max: %.1f V | Stall current @ V_eff: %.2f A\n', ...
        motor_Kt_NmA, motor_Ra_Ohm, V_max_eff_V, I_stall_eff_A);
    fprintf('Peak motor torque @ V_eff: %.3f N*m | Fuse (%.1f A) torque: %.3f N*m | Max speed: %.0f RPM\n', ...
        T_max_voltage_Nm, I_fuse_limit_A, T_max_fuse_Nm, w_max_eff_rpm);
    fprintf('Linear recoverable tilt from rest (V_eff): %.2f deg (static torque angle: %.2f deg)\n', ...
        theta_max_recoverable_deg, theta_max_static_voltage_deg);
    fprintf('45-deg jump-up: lift torque %.3f N*m vs motor %.3f N*m (e-brake %.3f N*m) | brake KE ratio: %.2fx\n', ...
        T_lift_45deg_Nm, T_max_voltage_Nm, T_ebrake_max_Nm, jump_up_energy_ratio);
    fprintf('4-state dlqr K (@ %.0f Hz): [%.3f, %.3f, %.4f, %.4f]\n', ...
        1/Ts_ctrl, K_lqr(1), K_lqr(2), K_lqr(3), K_lqr(4));
    fprintf('Closed-loop max |eig|: %.4f\n', max(abs(eig(sys_d.A - sys_d.B*K_lqr))));
end
