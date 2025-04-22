/*
################################################################################
# Описание бинарного протокола
################################################################################

По сети ходят пакеты вида
packet : = size payload

size - размер последовательности, в количестве элементов, может быть 0.

payload - поток байтов(blob)
payload состоит из последовательности сериализованных переменных разных типов :

Описание типов и порядок их сериализации

type : = id(uint64_t) data(blob)

data : =
    IntegerType - uint64_t
    FloatType - double
    StringType - size(uint64_t) blob
    VectorType - size(uint64_t) ...(сериализованные переменные)

Все данные передаются в little endian порядке байтов

Необходимо реализовать сущности IntegerType, FloatType, StringType, VectorType
Кроме того, реализовать вспомогательную сущность Any
Сделать объект Serialisator с указанным интерфейсом.

Конструкторы(ы) типов IntegerType, FloatType и StringType должны обеспечивать инициализацию, аналогичную инициализации типов uint64_t, double и std::string соответственно.
Конструктор(ы) типа VectorType должен позволять инициализировать его списком любых из перечисленных типов(IntegerType, FloatType, StringType, VectorType) переданных как по ссылке, так и по значению.
Все указанные сигнатуры должны быть реализованы.
Сигнатуры шаблонных конструкторов условны, их можно в конечной реализации делать на усмотрение разработчика, можно вообще убрать.
Vector::push должен быть именно шаблонной функцией. Принимает любой из типов: IntegerType, FloatType, StringType, VectorType.
Serialisator::push должен быть именно шаблонной функцией.Принимает любой из типов: IntegerType, FloatType, StringType, VectorType, Any
Реализация всех шаблонных функций, должна обеспечивать constraint requirements on template arguments, при этом, использование static_assert - запрещается.
Код в функции main не подлежит изменению. Можно только добавлять дополнительные проверки.

Архитектурные требования :
1. Стаедарт - c++17
2. Запрещаются виртуальные функции.
3. Запрещается множественное и виртуальное наследование.
4. Запрещается создание каких - либо объектов в куче, в том числе с использованием умных указателей.
   Это требование не влечет за собой огранечений на использование std::vector, std::string и тп.
5. Запрещается любое дублирование кода, такие случаи должны быть строго обоснованы. Максимально использовать обобщающие сущности.
   Например, если в каждой из реализаций XType будет свой IdType getId() - это будет считаться ошибкой.
6. Запрещается хранение value_type поля на уровне XType, оно должно быть вынесено в обобщающую сущность.
7. Никаких других ограничений не накладывается, в том числе на создание дополнительных обобщающих сущностей и хелперов.
8. XType должны реализовать сериализацию и десериализацию аналогичную Any.

Пример сериализации VectorType(StringType("qwerty"), IntegerType(100500))
{0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x71,0x77,0x65,0x72,0x74,0x79,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x94,0x88,
 0x01,0x00,0x00,0x00,0x00,0x00}
*/

#include <iostream>
#include <vector>
#include <fstream>

using Id = uint64_t;
using Buffer = std::vector<std::byte>;

enum class TypeId : Id {
    Uint,
    Float,
    String,
    Vector
};

class IntegerType {
public:
    template<typename ...Args>
    IntegerType(Args&& ...);
};

class FloatType {
public:
    template<typename ...Args>
    FloatType(Args&& ...);
};

class StringType {
public:
    template<typename ...Args>
    StringType(Args&& ...);
};

class VectorType {
public:
    template<typename ...Args>
    VectorType(Args&& ...);

    template<typename Arg>
    void push_back(Arg&& _val);
};

class Any {
public:
    template<typename ...Args>
    Any(Args&& ...);

    void serialize(Buffer& _buff) const;

    Buffer::const_iterator deserialize(Buffer::const_iterator _begin, Buffer::const_iterator _end);

    TypeId getPayloadTypeId() const;

    template<typename Type>
    auto& getValue() const;

    template<TypeId kId>
    auto& getValue() const;

    bool operator == (const Any& _o) const;
};

class Serializator {
public:
    template<typename Arg>
    void push(Arg&& _val);

    Buffer serialize() const;

    static std::vector<Any> deserialize(const Buffer& _val);

    const std::vector<Any>& getStorage() const;
};


int main() {

    std::ifstream raw;
    raw.open("raw.bin", std::ios_base::in | std::ios_base::binary);
    if (!raw.is_open())
        return 1;
    raw.seekg(0, std::ios_base::end);
    std::streamsize size = raw.tellg();
    raw.seekg(0, std::ios_base::beg);

    Buffer buff(size);
    raw.read(reinterpret_cast<char*>(buff.data()), size);

    auto res = Serializator::deserialize(buff);

    Serializator s;
    for (auto&& i : res)
        s.push(i);

    std::cout << (buff == s.serialize()) << '\n';

    return 0;
}