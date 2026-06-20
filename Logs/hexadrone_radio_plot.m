% --- 0. Housekeeping ---
close all;  % Closes all open figure windows
clear;      % Clears all variables from the workspace
clc;        % Clears the command window

% 1. Load Data
opts = detectImportOptions('HEXADRONE-2026-05-11-044332.csv');
opts.VariableNamingRule = 'preserve';
data = readtable('HEXADRONE-2026-05-11-044332.csv', opts);
t = duration(data.Time);

% 2. Visual Style Constants
F_TITLE = 14;   
F_LABEL = 12;   
F_LEGEND = 10;  

figure('Name', 'Gunslinger Mission Analysis', 'Color', 'w', 'Position', [100, 100, 1000, 800]);

% --- Subplot 1: Power & Voltage ---
ax(1) = subplot(4, 1, 1);
yyaxis left
plot(t, data.('Watt(W)'), 'Color', [0.8, 0, 0], 'LineWidth', 1.6); % Deep Red
ylabel('Power (W)', 'FontSize', F_LABEL);
ax(1).YAxis(1).Color = [0.8, 0, 0];

yyaxis right
plot(t, data.('RxBt(V)'), 'Color', [0, 0.4, 0.8], 'LineWidth', 1.6); % Bright Blue
ylabel('Voltage (V)', 'FontSize', F_LABEL);
ax(1).YAxis(2).Color = [0, 0.4, 0.8];
title('System Power & Battery Voltage', 'FontSize', F_TITLE);
grid on; set(gca, 'FontSize', F_LEGEND);

% --- Subplot 2: Pitch & Roll (Body Lean) ---
ax(2) = subplot(4, 1, 2);
plot(t, data.Ele, 'Color', [1, 0, 1], 'LineWidth', 1.5, 'DisplayName', 'Pitch'); % Magenta Solid
hold on;
plot(t, data.Ail, ':', 'Color', [0, 0.7, 0.7], 'LineWidth', 2.0, 'DisplayName', 'Roll'); % Teal Dotted
ylabel('Stick Units', 'FontSize', F_LABEL);
legend('Location', 'northeast', 'FontSize', F_LEGEND);
title('Body Lean Control', 'FontSize', F_TITLE);
grid on; set(gca, 'FontSize', F_LEGEND);

% --- Subplot 3: Gait & Switches (Enhanced Uniqueness) ---
ax(3) = subplot(4, 1, 3);
yyaxis left
plot(t, data.Thr, 'Color', [0.0, 0.3, 0.9], 'LineWidth', 2.0, 'DisplayName', 'Throttle'); 
hold on;
plot(t, data.Rud, ':', 'Color', [0, 0.6, 0], 'LineWidth', 2.0, 'DisplayName', 'Yaw'); 
ylabel('Gait/Yaw Units', 'FontSize', F_LABEL);
ax(3).YAxis(1).Color = [0.3, 0.3, 0.3];
yyaxis right
stairs(t, data.SB, 'Color', [1, 0.5, 0], 'LineWidth', 2.0, 'DisplayName', 'Posture (SB)'); 
hold on;
stairs(t, data.SC, 'Color', [0.5, 0, 0.5], 'LineWidth', 2.0, 'DisplayName', 'Gear (SC)'); 
ylabel('States', 'FontSize', F_LABEL);
ax(3).YAxis(2).Color = [0.4, 0, 0.4];
ylim([-1.5 1.5]);
legend('Location', 'northeast', 'FontSize', F_LEGEND);
title('Movement Magnitude & Operational States', 'FontSize', F_TITLE);
grid on; set(gca, 'FontSize', F_LEGEND);

% --- Subplot 4: Capacity ---
ax(4) = subplot(4, 1, 4);
area(t, data.('Capa(mAh)'), 'FaceColor', [0.2, 0.6, 1.0], 'FaceAlpha', 0.2, 'EdgeColor', [0, 0.4, 0.8]);
ylabel('Capacity (mAh)', 'FontSize', F_LABEL);
xlabel('Mission Time', 'FontSize', F_LABEL);
title('Energy Consumption', 'FontSize', F_TITLE);
grid on; set(gca, 'FontSize', F_LEGEND);

linkaxes(ax, 'x');
