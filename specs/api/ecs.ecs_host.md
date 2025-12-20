# API - ECS::ECSHost

## GetComponentStorageId
### API
- ```cpp
  size_t GetComponentStorageId(const std::string& component) const
  ```
- ```cpp
  size_t GetComponentStorageId(const char* component) const
  ```
### Behavior
- コンポーネントのIDを返す
- 対応するコンポーネントが存在しなかった場合、新しいIDを生成して返す
### Return
- コンポーネントのID

## NewEntity
### API
- ```cpp
  bool NewEntity(
    Types::Entity& entity, 
    const std::vector<std::string>& components, 
    size_t idMin = 0, 
    size_t idMax = std::numeric_limits<size_t>::max())
  ```
- ```cpp
  bool NewEntity(
    Types::Entity& entity, 
    const char** components, const size_t elementCount
    size_t idMin = 0, 
    size_t idMax = std::numeric_limits<size_t>::max())
  ```
### Behavior
- 新しいエンティティを作成し、引数`entity`に書き込む
- idの範囲を指定した場合、その範囲でエンティティを作成する
### Return
- このメソッドが成功したとき、`true`を返す
- このメソッドが失敗したとき、`false`を返す

## RemoveEntity
### API
- ```cpp
  bool RemoveEntity(const Types::Entity& entity)
  ```
### Behavior
- エンティティを削除する
### Return
- このメソッドが成功したとき、trueを返す
- このメソッドが失敗したとき、falseを返す

## AddComponent
### API
- ```cpp
  bool AddComponent(const Types::Entity& entity, const std::string& component)
  ```
- ```cpp
  bool AddComponent(const Types::Entity& entity, const char* component)
  ```
### Behavior
- `entity`にコンポーネントを追加する
### Return
- このメソッドが成功したとき、trueを返す
- このメソッドが失敗したとき、falseを返す

## RemoveComponent
### API
- ```cpp
  bool RemoveComponent(const Types::Entity& entity, const std::string& component)
  ```
- ```cpp
  bool RemoveComponent(const Types::Entity& entity, const char* component)
  ```
### Behavior
- `entity`に割り当てたコンポーネント`component`を削除する
### Return
- このメソッドが成功したとき、trueを返す
- このメソッドが失敗したとき、falseを返す
  
## NewComponentStorage
### API
- ```cpp
  template <Concepts::ComponentType T> bool NewComponentStorage(const std::string& id)
  ```
### Behavior
- 新しいコンポーネントストレージを追加する
### Return
- このメソッドが成功したとき、trueを返す
- このメソッドが失敗したとき、falseを返す

## GetComponentStorageAs
### API
- ```cpp
  template <Concepts::ComponentType T> ComponentStorage<T>* GetComponentStorageAs(const size_t index) const
  ```
- ```cpp
  template <Concepts::ComponentType T> ComponentStorage<T>* GetComponentStorageAs(const std::string& id) const
  ```
### Behavior
- `index`/`id`に対応するコンポーネントストレージを、`ComponentStorage<T>`のポインタとして返す
### Return
- このメソッドが成功したとき、対応するコンポーネントストレージのポインタを返す
- このメソッドが失敗したとき、`nullptr`を返す

## AddSystem
### API
- ```cpp
  template<typename T>
    requires std::derived_from<T, System::Base>
  size_t AddSystem()
  ```
### Behavior
- システム`T`を追加する
### Return
- このメソッドが成功したとき、追加したシステムのインデックスを返す
- このメソッドが失敗したとき、`std::numeric_limits<size_t>::max()`を返す