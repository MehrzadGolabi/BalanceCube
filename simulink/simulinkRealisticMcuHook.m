function y = simulinkRealisticMcuHook(u)
%SIMULINKREALISTICMCUHOOK Compiled C99 SIL hook with boot calib, stiction comp, 275-CPR encoder, & 45-deg IMU.
%   u = [theta_rad; theta_dot_rads; motor_shaft_w_rads; sim_time_s]
%   y = [V_cmd; mcu_mode]

    persistent cfgSim phiAccum prevTicks

    th = u(1);
    thd = u(2);
    wMotorShaft = u(3);
    tNow = u(4);

    if isempty(cfgSim) || (tNow < 0.0025)
        cfgSim = defaultControllerConfig();
        cfgSim.skipCalib = false;          % Run real 2.0 s boot gyro calibration!
        cfgSim.Vdeadband = single(0.42);   % Compensate ~10 mN*m brush/gearbox stiction
        phiAccum = 0.0;
        prevTicks = int32(0);
        isStart = true;
    else
        isStart = false;
    end

    Ts = double(cfgSim.Ts);

    % Quantize motor shaft angle to 4x quadrature integer ticks (275 ticks/rev)
    if ~isStart
        phiAccum = phiAccum + wMotorShaft * Ts;
    end
    ticksNow = int32(round(phiAccum * (double(cfgSim.cpr4x) / (2.0 * pi))));
    encTicksDelta = ticksNow - prevTicks;
    prevTicks = ticksNow;

    % Synthesize 45-deg PCB-rotated MPU6050 accel + biased/noisy gyro
    gAcc = 9.80665;
    axDiag = gAcc * sin(th);
    ayDiag = gAcc * cos(th);
    c45 = 0.70710678;

    noiseAx = 0.04 * sin(137.0 * tNow);
    noiseAy = 0.04 * cos(211.0 * tNow);
    trueGyroBias = 0.040; % +0.040 rad/s (~2.3 deg/s) uncalibrated bias removed at boot
    noiseGz = 0.0025 * sin(173.0 * tNow);

    axPcb = single(c45 * (axDiag + ayDiag) + noiseAx);
    ayPcb = single(c45 * (-axDiag + ayDiag) + noiseAy);
    gzMeas = single(thd + trueGyroBias + noiseGz);

    [pwmDuty, in1High, in2High, modeOut, ~, ~] = balanceCubeControllerStep_sil( ...
        axPcb, ayPcb, gzMeas, encTicksDelta, isStart, cfgSim);

    vCmd = (double(in1High) - double(in2High)) * double(pwmDuty) * double(cfgSim.VmaxEff);
    y = [vCmd; double(modeOut)];
end
