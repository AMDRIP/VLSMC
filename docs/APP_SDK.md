# RAND Elecorner 36 — App SDK (VLSMC API)

> Документация для разработчиков приложений
> Build 0001

---

## Обзор

Приложения для RAND Elecorner 36 пишутся на **TypeScript**, компилируются в JavaScript и исполняются внутри встроенного JS-движка (QJSEngine).

Каждое приложение получает доступ к глобальному объекту `VLSMC`, через который взаимодействует с ОС.

---

## Быстрый старт

```typescript
// main.ts — Пример приложения «Hello World»
const window = VLSMC.ui.createWindow({
  title: "Привет, мир!",
  width: 400,
  height: 300
});

window.setContent(`
  <text x="20" y="40" size="18">Добро пожаловать в RAND Elecorner 36!</text>
  <button id="btn" x="20" y="80" width="150" height="40">Нажми меня</button>
`);

window.on("click", "btn", () => {
  VLSMC.ui.alert("Кнопка нажата!");
});
```

```bash
# Компиляция и упаковка
tsc main.ts --target ES2020 --outDir dist/
vlsmc-pack --name "Hello World" --main dist/main.js --out hello.vlsmc-pkg
```

---

## API Reference

### `VLSMC.ui` — Пользовательский интерфейс

#### `createWindow(options): Window`

Создаёт новое окно приложения.

```typescript
interface WindowOptions {
  title: string;
  width: number;
  height: number;
  resizable?: boolean;   // default: true
  minWidth?: number;
  minHeight?: number;
}

interface Window {
  setContent(markup: string): void;
  setTitle(title: string): void;
  close(): void;
  minimize(): void;
  on(event: string, targetId: string, callback: Function): void;
  onClose(callback: Function): void;
}
```

#### `alert(message: string): void`
Показывает модальное диалоговое окно.

#### `confirm(message: string): boolean`
Показывает диалог подтверждения.

#### `prompt(message: string, defaultValue?: string): string | null`
Показывает диалог ввода.

#### `notify(message: string, type?: "info" | "warning" | "error"): void`
Показывает уведомление (toast).

---

### `VLSMC.fs` — Файловая система

Работает только в рамках разрешений, указанных в `manifest.json`.

```typescript
interface FileSystem {
  readFile(path: string): string;
  writeFile(path: string, content: string): void;
  exists(path: string): boolean;
  remove(path: string): void;
  rename(oldPath: string, newPath: string): void;
  mkdir(path: string): void;
  readdir(path: string): string[];
  stat(path: string): FileStat;
}

interface FileStat {
  name: string;
  size: number;
  isDirectory: boolean;
  permissions: string;      // "rwxr-xr--"
  owner: string;
  createdAt: number;        // timestamp
  modifiedAt: number;
}
```

**Ограничения:** приложение может работать только с:
- `/home/<user>/` — домашний каталог (если есть `filesystem.read` / `filesystem.write`)
- `/apps/<appId>/data/` — собственная директория данных (всегда доступна)

---

### `VLSMC.process` — Процессы

```typescript
interface ProcessAPI {
  info(): ProcessInfo;
  exit(code?: number): void;
  sleep(ticks: number): Promise<void>;
  spawn(command: string): number;  // возвращает PID
  list(): ProcessInfo[];
}

interface ProcessInfo {
  pid: number;
  name: string;
  state: "new" | "ready" | "running" | "waiting" | "terminated";
  priority: number;
  memoryUsed: number;
  cpuTime: number;
  owner: string;
}
```

---

### `VLSMC.ipc` — Межпроцессное взаимодействие

```typescript
interface IPCAPI {
  sendMessage(targetPid: number, data: any): void;
  onMessage(callback: (fromPid: number, data: any) => void): void;

  createPipe(): { read: () => string; write: (data: string) => void };

  acquireSemaphore(name: string): Promise<void>;
  releaseSemaphore(name: string): void;
}
```

---

### `VLSMC.system` — Системная информация

```typescript
interface SystemAPI {
  hostname(): string;            // "rand-elecorner-36"
  version(): string;             // "Build 0001"
  uptime(): number;              // тики с момента загрузки
  currentUser(): string;
  cpuUsage(): number;            // 0.0 — 1.0
  memoryUsage(): MemoryUsage;
  time(): number;                // текущий системный тик
}

interface MemoryUsage {
  total: number;
  used: number;
  free: number;
  percentage: number;
}
```

---

### `VLSMC.log` — Журналирование

```typescript
interface LogAPI {
  info(message: string): void;
  warn(message: string): void;
  error(message: string): void;
  debug(message: string): void;
}
```

---

## Разрешения (Permissions)

Приложение объявляет необходимые разрешения в `manifest.json`. Пользователь подтверждает их при установке.

| Разрешение | Доступ |
|-----------|--------|
| `ui.window` | Создание окон (требуется почти всегда) |
| `ui.notify` | Показ уведомлений |
| `filesystem.read` | Чтение файлов в `/home/<user>/` |
| `filesystem.write` | Запись файлов в `/home/<user>/` |
| `process.spawn` | Создание новых процессов |
| `process.list` | Просмотр списка всех процессов |
| `ipc.send` | Отправка сообщений другим процессам |
| `ipc.receive` | Получение сообщений от других процессов |
| `system.info` | Доступ к системной информации |

> Доступ к `/apps/<appId>/data/` и `VLSMC.log` не требует разрешений.

---

## Ограничения песочницы

| Ограничение | Значение |
|------------|---------|
| Максимальная память приложения | 16 МБ (настраиваемо) |
| Максимальное время выполнения (на тик) | 10 мс |
| Доступ к хост-ФС | ❌ Запрещён |
| Сетевые запросы | ❌ Запрещены (v1) |
| Доступ к `eval()` / `Function()` | ❌ Запрещён |
| Рекурсия | Ограничена глубиной 256 |
