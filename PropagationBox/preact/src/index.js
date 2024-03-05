import './style';
import { useEffect, useState } from 'preact/hooks';

export default function App() {
  return (
    <div className="app-root">
      <h1>Propagation box</h1>
      <hr />
      <SensorInfo />
      <hr />
      <Peripherals />
      <hr />
      <EnvironmentalControls />
      <hr />
      <GlobalInfo />
      <hr />
      <WifiForm />
      <hr />
      <Restart />
    </div>
  );
}

const GlobalInfo = () => {
  const [globalInfo, setGlobalInfo] = useState({});

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
      <p>Current time: {globalInfo.CurrentTime}</p>
    </Section>
  );
};

const SensorInfo = () => {
  const [sensorInfo, setSensorInfo] = useState({});

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
      <p>Air Temperature: {cToF(sensorInfo.air_temperature)}F</p>
      <p>Humidity: {sensorInfo.humidity}%</p>
      <p>Probe Temperature: {cToF(sensorInfo.probe_temperature)}F</p>
    </Section>
  );
};

const Peripherals = () => {
  const [peripherals, setPeripherals] = useState({});
  useEffect(() => {
    const load = async () => {
      const data = await fetch('/peripherals');
      const json = await data.json();
      setPeripherals(json);
    };
    load();
  }, []);

  return (
    <Section className="Peripherals" title="Peripherals">
      <p>Heat Mat: {peripherals.heat_mat}</p>
      <p>Fan: {peripherals.fan}</p>
      <p>LED Level: {peripherals.led_level}</p>
    </Section>
  );
};

const EnvironmentalControls = () => {
  const [environmentalControls, setEnvironmentalControls] = useState({});
  const [isLoading, setIsLoading] = useState(false);

  useEffect(() => {
    const load = async () => {
      const data = await fetch('/environmental-controls');
      const json = await data.json();
      setEnvironmentalControls(json);
    };
    load();
  }, []);

  const handleSubmit = async () => {
    setIsLoading(true);
    try {
      const data = new FormData();
      for (const [key, value] of Object.entries(environmentalControls)) {
        data.append(key, value);
      }
      const response = await fetch('/environmental-controls', {
        method: 'POST',
        body: data,
      });
      const json = await response.json();

      setEnvironmentalControls(json);
    } catch (_error) {
      alert('Error submitting form');
    }
    setIsLoading(false);
  };

  const formatTime = (minutes) => {
    const hour = Math.floor(minutes / 60);
    const minute = minutes % 60;
    const hourString = hour < 10 ? `0${hour}` : `${hour}`;
    const minuteString = minute < 10 ? `0${minute}` : `${minute}`;
    return `${hourString}:${minuteString}`;
  };

  const formatTimeToMinutes = (time) => {
    const [hour, minute] = time.split(':');
    return parseInt(hour) * 60 + parseInt(minute);
  };

  const onTime = formatTime(environmentalControls.on_time);
  const offTime = formatTime(environmentalControls.off_time);
  const naturalLight =
    environmentalControls.natural_light === '1' ? true : false;

  return (
    <Section
      className={`EnvironmentalControls ${isLoading ? 'loading' : ''}`}
      title="Environmental Controls"
    >
      <p>
        <label for="desired_temp">Desired Temperature</label>
        <input
          type="number"
          id="desired_temp"
          name="desired_temp"
          value={environmentalControls.desired_temp}
          onChange={(e) =>
            setEnvironmentalControls({
              ...environmentalControls,
              desired_temp: e.target.value,
            })
          }
        />
      </p>
      <p>
        <label for="temp_range">Temperature Range</label>
        <input
          type="number"
          id="temp_range"
          name="temp_range"
          value={environmentalControls.temp_range}
          onChange={(e) =>
            setEnvironmentalControls({
              ...environmentalControls,
              temp_range: e.target.value,
            })
          }
        />
      </p>
      <p>
        <label for="desired_humidity">Desired Humidity</label>
        <input
          type="number"
          id="desired_humidity"
          name="desired_humidity"
          value={environmentalControls.desired_humidity}
          onChange={(e) =>
            setEnvironmentalControls({
              ...environmentalControls,
              desired_humidity: e.target.value,
            })
          }
        />
      </p>
      <p>
        <label for="humidity_range">Humidity Range</label>
        <input
          type="number"
          id="humidity_range"
          name="humidity_range"
          value={environmentalControls.humidity_range}
          onChange={(e) =>
            setEnvironmentalControls({
              ...environmentalControls,
              humidity_range: e.target.value,
            })
          }
        />
      </p>
      <p>
        <label for="natural_light">Natural Light</label>
        <input
          type="checkbox"
          id="natural_light"
          name="natural_light"
          checked={naturalLight}
          onChange={(e) =>
            setEnvironmentalControls({
              ...environmentalControls,
              natural_light: e.target.checked ? '1' : '0',
            })
          }
        />
      </p>
      <p>
        <label for="on_time">On Time</label>
        <input
          type="time"
          id="on_time"
          name="on_time"
          value={onTime}
          onChange={(e) =>
            setEnvironmentalControls({
              ...environmentalControls,
              on_time: formatTimeToMinutes(e.target.value),
            })
          }
        />
      </p>
      <p>
        <label for="off_time">Off Time</label>
        <input
          type="time"
          id="off_time"
          name="off_time"
          value={offTime}
          onChange={(e) =>
            setEnvironmentalControls({
              ...environmentalControls,
              off_time: formatTimeToMinutes(e.target.value),
            })
          }
        />
      </p>
      <button onClick={() => handleSubmit()}>Submit</button>
    </Section>
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
            await fetch('/restart', { method: 'POST' });
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

const Section = ({ className = '', title, children }) => {
  return (
    <div className={`Section ${className}`}>
      <h2>{title}</h2>
      {children}
    </div>
  );
};

const cToF = (c) => (c === undefined ? '' : ((+c * 9) / 5 + 32).toFixed(0));
