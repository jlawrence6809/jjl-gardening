import { Section } from './components/Section';

export const WifiForm = () => {
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
