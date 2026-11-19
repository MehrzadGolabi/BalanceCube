function tauDist = handAndKnockDisturbance(u)
%HANDANDKNOCKDISTURBANCE 12-second bench lifecycle: calib -> hand lift -> tap -> knockdown -> re-arm.
%   u = [theta_rad; theta_dot_rads; mcu_mode; sim_time_s]

    th = u(1);
    thd = u(2);
    mode = round(u(3));
    t = u(4);

    tauDist = 0.0;

    if (t >= 2.1) && (t < 4.8) && (mode ~= 2)
        % Smooth hand lift from +45 deg table rest to +1.0 deg engage pose
        s = min(1.0, (t - 2.1) / 0.55);
        thRef = deg2rad(45.0) * (1.0 - s) + deg2rad(1.0) * s;
        tauDist = -15.0 * (th - thRef) - 1.5 * thd;
        tauDist = max(-1.2, min(1.2, tauDist));
    elseif (t >= 5.0) && (t < 5.1)
        % Gentle tap disturbance during steady balancing
        tauDist = 0.04;
    elseif (t >= 7.0) && (t < 7.25)
        % Hard shove knocking cube past 30 deg fall cutoff onto 45 deg stop
        tauDist = 0.45;
    elseif (t >= 9.0) && (t < 11.5) && (mode ~= 2)
        % Second hand lift from +45 deg fault rest to -1.0 deg -> auto re-arm
        s = min(1.0, (t - 9.0) / 0.55);
        thRef = deg2rad(45.0) * (1.0 - s) + deg2rad(-1.0) * s;
        tauDist = -15.0 * (th - thRef) - 1.5 * thd;
        tauDist = max(-1.2, min(1.2, tauDist));
    end
end
