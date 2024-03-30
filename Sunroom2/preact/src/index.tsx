import './style';
import { useEffect, useState } from 'preact/hooks';
import { createPortal } from 'preact/compat';
import { VNode } from 'preact';
import { parseInputString, Err, ParsedRule } from './RuleParser';

const RELAY_COUNT = 8 as const;
const RELAY_LIST = new Array(RELAY_COUNT).fill(0).map((_, i) => `relay_${i}`);

const PORTAL_ROOT_ID = 'portal-root';

export default function App() {
  const name = getName();

  // set tab name:
  useEffect(() => {
    document.title = `${name}`;
  }, [name]);

  return (
    <>
      <div className="app-root">
        <h1>{name}</h1>
        <hr />
        <RelayControls />
        <hr />
        <GlobalInfo />
        <hr />
        <SensorInfo />
        <hr />
        <WifiForm />
        <hr />
        <Restart />
      </div>

      <div id={PORTAL_ROOT_ID}></div>
    </>
  );
}

type GlobalInfoResponse = {
  ChipId?: string;
  ResetCounter?: number;
  InternalTemperature?: number;
  CurrentTime?: string;
};

const GlobalInfo = () => {
  const [globalInfo, setGlobalInfo] = useState<GlobalInfoResponse>({});

  useEffect(() => {
    const load = async () => {
      const data = await fetch('/global-info');
      const json = await data.json();
      setGlobalInfo(json);
    };
    load();
  }, []);

  return (
    <Section className="GlobalInfo" title="Global Info">
      <p>Chip Id: #{globalInfo.ChipId}</p>
      <p>Resets: {globalInfo.ResetCounter}</p>
      <p>Internal temperature: {globalInfo.InternalTemperature}F</p>
      <p>Current time: {globalInfo.CurrentTime}</p>
    </Section>
  );
};

type SensorInfoResponse = {
  Temperature?: number;
  Humidity?: number;
  Light?: number;
  Switch?: number;
};

const SensorInfo = () => {
  const [sensorInfo, setSensorInfo] = useState<SensorInfoResponse>({});

  useEffect(() => {
    const load = async () => {
      const data = await fetch('/sensor-info');
      const json = await data.json();
      setSensorInfo(json);
    };
    load();
  }, []);

  return (
    <Section className="SensorInfo" title="Sensor Info">
      <p>Temperature: {sensorInfo.Temperature}F</p>
      <p>Humidity: {sensorInfo.Humidity}%</p>
      <p>Light: {sensorInfo.Light}</p>
      <p>Switch: {sensorInfo.Switch}</p>
    </Section>
  );
};

/*
 * 0 = off, 1 = on, 2 = x/dont care
 */
type RelaySubState = 0 | 1 | 2;

/**
 * Ones digit: force digit (0 = off, 1 = on, 2 = x/dont care)
 * Tens digit: auto digit (0 = off, 1 = on, 2 = x/dont care)
 */
type RelaySubmissionValue = `${RelaySubState}${RelaySubState}`;
type RelayStateValue = {
  force: RelaySubState;
  auto: RelaySubState;
};
type Relay = `relay_${number}`;

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

const RelayControls = () => {
  const [relayState, setRelayState] = useState<
    Record<Relay, RelayStateValue | 'loading'>
  >(() =>
    RELAY_LIST.reduce((acc, relay) => ({ ...acc, [relay]: 'loading' }), {}),
  );

  const [relayLabels, setRelayLabels] = useState<Record<Relay, string>>(
    RELAY_LIST.reduce((acc, relay) => ({ ...acc, [relay]: '' }), {}),
  );

  const [automateDialogRelay, setAutomateDialogRelay] = useState<Relay | null>(
    null,
  );

  const isLoading = Object.values(relayState).some(
    (value) => value === 'loading',
  );

  const fetchRelays = async () => {
    const data = await fetch('/relays');
    const json = await data.json();
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

  return (
    <Section
      className={`RelayForm ${isLoading ? 'loading' : ''}`}
      title="Relay Controls"
    >
      {new Array(8).fill(0).map((_, i) => {
        const relay = `relay_${i}` as Relay;
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

type AutomateDialogProps = {
  relay: Relay | null;
  label: string | undefined;
  setLabel: (label: string) => void;
  onClose: (refreshRelays: boolean) => void;
};

const AutomateDialog = ({
  relay,
  label,
  setLabel,
  onClose,
}: AutomateDialogProps) => {
  const [rule, setRule] = useState<string>('loading');
  const [validationResult, setValidationResult] = useState<ParsedRule | Err>(
    null,
  );
  const submitDisabled =
    !validationResult || (validationResult as Err)?.type === 'ERROR';

  const relayIdx = relay === null ? null : parseInt(relay?.split('_')?.[1]);

  useEffect(() => {
    const load = async () => {
      if (relayIdx === null) return;
      const data = await fetch(`/rule?i=${relayIdx}`);
      const json = await data.json();
      setRule(json.v);
    };
    load();
  }, [relayIdx]);

  if (relay === null) return <></>;

  const submit = async () => {
    if (submitDisabled) {
      return;
    }
    try {
      const formData = new FormData();
      formData.append('v', JSON.stringify(JSON.parse(rule)));
      formData.append('i', relayIdx.toString());

      await fetch(`/rule`, {
        method: 'POST',
        body: formData,
      });
      onClose(true);
    } catch (error) {
      console.error(error);
      alert('Error submitting rule');
    }
  };

  return (
    <FullScreenDialog onClose={() => onClose(false)}>
      <div className="AutomateDialog">
        <h3>
          <span
            contentEditable
            onBlur={(ev) => setLabel(ev.currentTarget.textContent.trim())}
          >
            {label}
          </span>
          <sup className="Pencil">✏️</sup>
        </h3>
        <textarea
          value={rule}
          disabled={rule === 'loading'}
          onChange={(ev) => setRule(ev.currentTarget.value)}
          style={{ width: '100%', height: '200px' }}
        ></textarea>

        <div className="Buttons">
          <button onClick={() => setValidationResult(parseInputString(rule))}>
            Validate
          </button>
          <button onClick={() => submit()} disabled={submitDisabled}>
            Submit
          </button>
        </div>

        {/* display validation result */}
        {validationResult && (
          <div>
            <h4>Validation Result</h4>
            <pre>{JSON.stringify(validationResult, null, 2)}</pre>
          </div>
        )}

        {/* display current sensor actuator values */}
      </div>
    </FullScreenDialog>
  );
};

const WifiForm = () => {
  return (
    <Section title="Wifi Settings">
      <form className="WifiForm" action="/wifi-settings" method="post">
        <div>
          <label for="ssid">Wifi Name</label>
          <input type="text" id="ssid" name="ssid" />
        </div>
        <div>
          <label for="password">Wifi Password</label>
          <input type="password" id="password" name="password" />
        </div>
        <button type="submit">Submit</button>
      </form>
    </Section>
  );
};

const RESTART_SECONDS = 5;

const Restart = () => {
  const [restartSuccess, setRestartSuccess] = useState(false);

  return (
    <div>
      <button
        onClick={async () => {
          try {
            await fetch('/reset', { method: 'POST' });
            setRestartSuccess(true);
            setTimeout(() => {
              setRestartSuccess(false);
            }, RESTART_SECONDS * 1000);
          } catch (error) {
            console.error(error);
            alert('Error restarting');
          }
        }}
      >
        Reset
      </button>
      {restartSuccess && (
        <p>Success, restarting in {RESTART_SECONDS} seconds...</p>
      )}
    </div>
  );
};

type SectionProps = {
  className?: string;
  title: string;
  children: any;
};

const Section = ({ className = '', title, children }: SectionProps) => {
  return (
    <div className={`Section ${className}`}>
      <h2>{title}</h2>
      {children}
    </div>
  );
};

type FullScreenDialogProps = {
  children: preact.ComponentChild;
  onClose: () => void;
};

const FullScreenDialog = ({ children, onClose }: FullScreenDialogProps) => {
  return (
    <DialogPortal>
      <div className="FullScreenDialog" onClick={onClose}>
        <div className="inner" onClick={(e) => e.stopPropagation()}>
          {children}
        </div>
      </div>
    </DialogPortal>
  );
};

const DialogPortal = ({ children }: { children: VNode<{}> }) => {
  const portalRoot = document.getElementById(PORTAL_ROOT_ID);
  return portalRoot ? createPortal(children, portalRoot) : null;
};

/**
 * Get the domain name of the current page.
 */
const getName = () => {
  // get domain name: eg. http://sunroom2.local -> sunroom2

  const rawDomain = window.location.hostname.split('.')?.[0];

  if (!rawDomain) {
    return 'Unknown';
  }

  // capitalize first letter
  return rawDomain.charAt(0)?.toUpperCase() + rawDomain.slice(1);
};
