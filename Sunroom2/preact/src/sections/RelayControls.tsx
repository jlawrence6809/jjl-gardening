import { useEffect, useState } from 'preact/hooks';
import { Section } from '../components/Section';
import { AutomateDialog } from '../components/AutomateDialog';
import {
  Relay,
  RelayStateValue,
  RelaySubmissionValue,
  RelaySubState,
} from '../types';

/**
 * Rule parse needs to know the number of relays. Default to 0 until we load the relays.
 */
export let RELAY_COUNT = 0;

/**
 * Convert from the relay state submission value to the relay state value.
 */
const getRelayStateValues = (
  blob: Record<Relay, RelaySubmissionValue>,
): Record<Relay, RelayStateValue> => {
  return Object.fromEntries(
    Object.entries(blob).map(([key, rawValue]) => {
      const value = parseInt(rawValue);
      const onesDigit = (value % 10) as RelaySubState;
      const tensDigit = Math.floor(value / 10) as RelaySubState;
      return [key, { force: onesDigit, auto: tensDigit }];
    }),
  ) as Record<Relay, RelayStateValue>;
};

/**
 * This is the order:
 * force off -> force on -> force x -> force off -> etc...
 * So do not modify the tens digit, just the ones digit (force digit).
 */
const getNextRelayState = (current: RelayStateValue): RelayStateValue => {
  const { force, auto } = current;
  const nextForce = ((force + 1) % 3) as RelaySubState;
  return { force: nextForce, auto };
};

const getRelaySubmissionValue = (
  current: RelayStateValue,
): RelaySubmissionValue =>
  `${current.auto}${current.force}` as RelaySubmissionValue;

export const RelayControls = () => {
  const [relayState, setRelayState] = useState<
    Record<Relay, RelayStateValue | 'loading'> | 'loading'
  >('loading');

  const [relayLabels, setRelayLabels] = useState<Record<Relay, string>>({});
  const [gpioOptions, setGpioOptions] = useState<number[] | 'loading'>(
    'loading',
  );
  const [adding, setAdding] = useState(false);
  const [newPin, setNewPin] = useState<number | null>(null);
  const [newInverted, setNewInverted] = useState(true);

  const [automateDialogRelay, setAutomateDialogRelay] = useState<Relay | null>(
    null,
  );

  const isLoading = Object.values(relayState).some(
    (value) => value === 'loading',
  );

  const fetchRelays = async () => {
    const data = await fetch('/relays');
    const json = await data.json();
    RELAY_COUNT = Object.keys(json).length;
    setRelayState(getRelayStateValues(json));
  };

  useEffect(() => {
    fetchRelays();
  }, []);

  useEffect(() => {
    const load = async () => {
      const data = await fetch('/relay-labels');
      const json = await data.json();
      setRelayLabels(json);
    };
    load();
  }, []);

  const refreshGpioOptions = async () => {
    setGpioOptions('loading');
    const data = await fetch('/gpio-options');
    const json = await data.json();
    const options = Object.keys(json).map((k) => parseInt(k, 10));
    setGpioOptions(options);
  };
  useEffect(() => {
    refreshGpioOptions();
  }, []);

  const updateRelayLabel = async (label: string) => {
    if (!automateDialogRelay) return;

    const oldLabel = relayLabels[automateDialogRelay];
    setRelayLabels({ ...relayLabels, [automateDialogRelay]: 'Updating...' });

    try {
      const formData = new FormData();
      formData.append('i', automateDialogRelay.split('_')[1]);
      formData.append('v', label);
      await fetch(`/relay-label`, {
        method: 'POST',
        body: formData,
      });
      setRelayLabels({ ...relayLabels, [automateDialogRelay]: label });
    } catch (error) {
      console.error(error);
      alert('Error updating relay label');
      setRelayLabels({ ...relayLabels, [automateDialogRelay]: oldLabel });
    }
  };

  const handleSubmit = async (relay: Relay, value: RelayStateValue) => {
    if (relayState === 'loading') return;
    const previousState = relayState[relay];
    if (previousState === 'loading') return;
    setRelayState({
      ...relayState,
      [relay]: 'loading',
    });

    try {
      const data = new FormData();
      data.append(relay, getRelaySubmissionValue(value));
      const response = await fetch('/relays', {
        method: 'POST',
        body: data,
      });
      const json = await response.json();
      setRelayState(getRelayStateValues(json));
    } catch (_error) {
      setRelayState({
        ...relayState,
        [relay]: previousState,
      });
    }
  };

  const addRelay = async () => {
    if (newPin == null) return;
    setAdding(true);
    try {
      const form = new FormData();
      form.append('pin', String(newPin));
      form.append('inv', newInverted ? '1' : '0');
      await fetch('/relay-config/add', { method: 'POST', body: form });
      await fetchRelays();
      await refreshGpioOptions();
      setNewPin(null);
    } finally {
      setAdding(false);
    }
  };

  return (
    <Section
      className={`RelayForm ${isLoading ? 'loading' : ''}`}
      title="Relay Controls"
    >
      {relayState === 'loading' && <div>Loading...</div>}
      {relayState !== 'loading' && Object.keys(relayState).length === 0 && (
        <div style={{ padding: '0.5rem 0' }}>No relays configured.</div>
      )}
      {relayState !== 'loading' &&
        Object.keys(relayState).map((relay: Relay) => {
          const value = relayState[relay];
          const label = relayLabels[relay];

          let stateClasses = 'loading';
          if (value !== 'loading') {
            stateClasses = `auto_${value.auto} force_${value.force}`;
          }

          return (
            <div
              key={relay}
              className={`ToggleSwitch ${stateClasses}`}
              onClick={() =>
                value !== 'loading' &&
                handleSubmit(relay, getNextRelayState(value))
              }
            >
              <div
                className={'AutomateButton'}
                onClick={(ev) => {
                  ev.stopPropagation();
                  setAutomateDialogRelay(relay);
                }}
              >
                ⛭
              </div>
              {label}
            </div>
          );
        })}
      <div style={{ marginTop: '1rem' }}>
        <h4>Add Relay</h4>
        <div style={{ display: 'flex', gap: '0.5rem', alignItems: 'center' }}>
          <select
            value={newPin == null ? '' : newPin}
            onChange={(e) =>
              setNewPin(
                e.currentTarget.value === ''
                  ? null
                  : parseInt(e.currentTarget.value, 10),
              )
            }
          >
            <option value="">Select GPIO</option>
            {gpioOptions !== 'loading' &&
              gpioOptions.map((p) => (
                <option key={p} value={p}>
                  GPIO {p}
                </option>
              ))}
          </select>
          <label
            style={{ display: 'flex', gap: '0.25rem', alignItems: 'center' }}
          >
            <input
              type="checkbox"
              checked={newInverted}
              onChange={(e) => setNewInverted(e.currentTarget.checked)}
            />
            Inverted
          </label>
          <button disabled={adding || newPin == null} onClick={addRelay}>
            {adding ? 'Adding...' : 'Add Relay'}
          </button>
        </div>
      </div>
      <AutomateDialog
        key={automateDialogRelay}
        relay={automateDialogRelay}
        label={relayLabels[automateDialogRelay]}
        setLabel={updateRelayLabel}
        onClose={(refreshRelays) => {
          setAutomateDialogRelay(null);
          if (refreshRelays) fetchRelays();
        }}
      />
    </Section>
  );
};
