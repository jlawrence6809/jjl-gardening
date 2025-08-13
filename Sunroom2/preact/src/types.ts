/*
 * 0 = off, 1 = on, 2 = x/dont care
 */
export type RelaySubState = 0 | 1 | 2;

/**
 * Relay submission value used on the backend/device.
 *
 * Ones digit: force digit (0 = off, 1 = on, 2 = x/dont care)
 * Tens digit: auto digit (0 = off, 1 = on, 2 = x/dont care)
 */
export type RelaySubmissionValue = `${RelaySubState}${RelaySubState}`;

/**
 * Relay state value for use in the UI.
 */
export type RelayStateValue = {
  force: RelaySubState;
  auto: RelaySubState;
};

/**
 * A relay is a single relay on the device.
 */
export type Relay = `relay_${number}`;
