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

--count_of_elements_in_packege = 1: vector
{0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
--vector_id = 3
 0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 --count_of_elements = 2
 0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 --string_id = 2
 0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 --size_string
 0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 q      w   e     r    t    y  --id_int=0
 0x71,0x77,0x65,0x72,0x74,0x79,0x00,0x00,
                               100500
 0x00,0x00,0x00,0x00,0x00,0x00,0x94,0x88,
100500
 0x01,0x00,0x00,0x00,0x00,0x00}
*/

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <fstream>
#include <type_traits>

using Id = uint64_t;
using Buffer = std::vector<std::byte>;

enum class TypeId : Id {
    Uint,
    Float,
    String,
    Vector
};

//Функции для чтения и записи значений в/из поток(а). Вынес отдельно т.к. используются в нескольких классах
template<typename T>   
T read_value(Buffer::const_iterator& begin, const Buffer::const_iterator end){
    T value;
    if(std::distance(begin, end) < sizeof(T)){
        std::runtime_error("invalid buffer");
    }
    std::memcpy(&value, &(*begin), sizeof(T));
    begin += sizeof(T);
    return value;
}

template<typename T>
void write_value(Buffer& buff, const T& value) {
    const auto old_size = buff.size();
    buff.resize(old_size + sizeof(T));
    std::memcpy(buff.data() + old_size, &value, sizeof(T));
}

//Базовый тип для всех значений. Хранит id объекта
class TypeBase{
public:
    TypeBase(){}
    TypeBase(TypeId id){_id = id;}
    TypeBase(const TypeBase& other){_id = other._id;}

    void serialize(Buffer& buff) const {
        write_value(buff, _id);
    }

    TypeId get_id() const {return _id;}

protected:
    TypeId _id;
};

//Шаблонынй класс, в который я вынес повторяющуюся логику хранения некоторого объекта и его сериализацию
template<typename T>
class XType:public TypeBase{
public:

    template<typename ...Args>
    XType(TypeId id, Args&& ... args):TypeBase(id),_value(std::forward<Args>(args)...){}//Передаём все параметры конструктора в конструктор T для создания Н. IntegerType, как uint64_t

    XType(const XType& other):TypeBase(other){
        _value = other._value;
    }

    T get_value() const {return _value;}

    void serialize(Buffer& buff) const{
        TypeBase::serialize(buff);
        write_value(buff, _value);
    }
    Buffer::const_iterator deserialize(Buffer::const_iterator begin, Buffer::const_iterator end){
        _value = read_value<T>(begin, end);
        return begin;
    }

    bool operator == (const XType<T>& _o) const{
        return _value == _o._value;
    }
protected:
    T _value;
};

//Классы IntegerType, FloatType, VectorType и StringType - просто конкретные классы XType<T>
class IntegerType:public XType<uint64_t> {
public:
    template<typename ...Args>
    IntegerType(Args&& ... args):XType<uint64_t>(TypeId::Uint, std::forward<Args>(args) ...)  {
    }
    IntegerType(const IntegerType& other):XType<uint64_t>(other){}
};

class FloatType:public XType<double> {
public:
    template<typename ...Args>
    FloatType(Args&& ... args):XType<double>(TypeId::Float, std::forward<Args>(args) ...){
    }
    
    FloatType(const FloatType& other):XType<double>(other){}
};

class StringType:public XType<std::string> {
public:
    template<typename ...Args>
    StringType(Args&& ... args):XType<std::string>(TypeId::String, std::forward<Args>(args) ...){
    }
    StringType(const StringType& other):XType<std::string>(other){}
    //строка сериализуется не так, как обычный тип, поэтому скрываем базовое решение
    void serialize(Buffer& buff) const{
        TypeBase::serialize(buff);
        write_value(buff, _value.size());

        auto old_size = buff.size();
        buff.resize(old_size+_value.size());
        std::memcpy(buff.data() + old_size, _value.c_str(), _value.size());
    }
    Buffer::const_iterator deserialize(Buffer::const_iterator begin, Buffer::const_iterator end){
        uint64_t size = read_value<uint64_t>(begin, end);
        std::vector<char> t(size);
        std::memcpy(t.data(), &(*begin), size);
        _value = std::string(t.data(), t.size());
        begin+=size;
        return begin;
    }
};

class Any;

class VectorType:public XType<std::vector<Any>> {
public:
    template<typename ...Args>
    VectorType(Args&& ... args):XType<std::vector<Any>>(TypeId::Vector, std::forward<Args>(args) ...){

    }
    VectorType(const VectorType& other):XType<std::vector<Any>>(other){
    }
    
    //Здесь 2 реализации конструкторов: если в условии подразумевался вектор с конкретными типами и если вектор с ссылками на разные типы
    //Немного не понял то условие и сделал 2 реализации

    //Шаблонные конструкторы для вектора типа std::vector<TypeBase*> и std::vector<TypeBase*>* 
    VectorType(const std::vector<TypeBase*> other);//Вынесены за класс Any т.к. активно с ним работают

    VectorType(const std::vector<TypeBase*>* other):VectorType(*other){
    }

    //Шаблонные конструкторы для вектора типа std::vector<IntegerType> и std::vector<IntegerType*> 
    template<typename T, typename = typename std::enable_if<
    std::is_same<T, IntegerType>::value ||
    std::is_same<T, FloatType>::value ||
    std::is_same_v<T, VectorType>||
    std::is_same<T, StringType>::value
    >::type>
    VectorType(const std::vector<T> other):XType<std::vector<Any>>(TypeId::Vector){
        for (auto item : other) {
            push_back(&item);
        }
    }
    
    template<typename T, typename = typename std::enable_if<
    std::is_same<T, IntegerType>::value ||
    std::is_same<T, FloatType>::value ||
    std::is_same_v<T, VectorType>||
    std::is_same<T, StringType>::value
    >::type>
    VectorType(const std::vector<T*> other):XType<std::vector<Any>>(TypeId::Vector){
        for (auto item : other) {
            push_back(item);
        }
    }

    //Реализация push_back
    template<typename T>
    typename std::enable_if<
    std::is_same_v<T, IntegerType> || 
    std::is_same_v<T, FloatType> ||
    std::is_same_v<T, VectorType>||
    std::is_same_v<T, StringType>, void
>::type
    push_back(T _val){

        _value.push_back(Any(&_val));
    }
    //Альтернатива с ссылкой
    void push_back(TypeBase* _val);

    void serialize(Buffer& buff) const;//Вынесены за класс Any т.к. активно с ним работают
    Buffer::const_iterator deserialize(Buffer::const_iterator begin, Buffer::const_iterator end);
};

class Any {
public:
    Any(){
    }

    Any(TypeBase* value){
        //т.к. нам запрещено работать с виртуальными функциями, я наплодил if else и static_cast к конкретным типам в этом классе
        //это некрасиво, но работает и я незнаю, как это сократить
        auto id = value->get_id();
        if(id == TypeId::Float){
            _float_type = FloatType{*static_cast<const FloatType*>(value)};
            _value = &_float_type;
        }
        else if (id == TypeId::String) {
            _str_type = StringType{*static_cast<const StringType*>(value)};
            _value = &_str_type;
        }
        else if (id == TypeId::Uint) {
            _int_type = IntegerType{*static_cast<const IntegerType*>(value)};
            _value = &_int_type;
        }
        else if (id == TypeId::Vector) {
            _vec_type = VectorType{*static_cast<const VectorType*>(value)};
            _value = &_vec_type;
        }
    }

    Any(const Any& other):Any(other._value) {}



    void serialize(Buffer& buff) const{
        auto id = getPayloadTypeId();
        if(id == TypeId::Float){
            _float_type.serialize(buff);
        }
        else if (id == TypeId::String) {
            _str_type.serialize(buff);
        }
        else if (id == TypeId::Uint) {
            _int_type.serialize(buff);
        }
        else if (id == TypeId::Vector) {
            _vec_type.serialize(buff);
        }
    }

    Buffer::const_iterator deserialize(Buffer::const_iterator begin, Buffer::const_iterator end){
        auto id = read_value<TypeId>(begin, end);
        if(id == TypeId::Float){
            _float_type = FloatType();
            begin = _float_type.deserialize(begin, end);
            _value = &_float_type;
        }
        else if (id == TypeId::String) {
            _str_type = StringType();
            begin = _str_type.deserialize(begin, end);
            _value = &_str_type;
        }
        else if (id == TypeId::Uint) {
            _int_type = IntegerType();
            begin = _int_type.deserialize(begin, end);
            _value = &_int_type;
        }
        else if (id == TypeId::Vector) {
            _vec_type = VectorType();
            begin = _vec_type.deserialize(begin, end);
            _value = &_vec_type;
        }
        return begin;
    }

    TypeId getPayloadTypeId() const{
        if(_value == nullptr){
            throw  std::runtime_error("value is null");
        }
        return _value->get_id();
        }

    template<typename Type>
    auto& getValue() const{
        if(_value == nullptr){
            throw  std::runtime_error("value is null");
        }
        return *static_cast<Type*>(_value);
        }

    template<TypeId kId>
    auto& getValue() const{
        if(_value == nullptr){
            throw  std::runtime_error("value is null");
        }
        return _value->get_id();
    }

    bool operator == (const Any& _o) const{
        auto id = getPayloadTypeId();
        if(_o.getPayloadTypeId() != id){
            return false;
        }
        if(id == TypeId::Float){
            return ((*static_cast<FloatType*>(_o._value)).get_value() == _float_type.get_value());
        }
        else if (id == TypeId::String) {
            return ((*static_cast<StringType*>(_o._value)).get_value() == _str_type.get_value());
        }
        else if (id == TypeId::Uint) {
            return ((*static_cast<IntegerType*>(_o._value)).get_value() == _int_type.get_value());
        }
        else if (id == TypeId::Vector) {
            return ((*static_cast<VectorType*>(_o._value)).get_value() == _vec_type.get_value());
        }
        return false;
    }
private:

    //Здесь храниться ссылка на объект, обвёрнутый в Any
    TypeBase* _value = nullptr;

    //Здесь хранятся 4 объекта в стеке, но используется только один т.к. при объявлении объекта в функции он стирается с функцией
    //а эти стираются вместе с классом
    IntegerType _int_type;
    FloatType _float_type;
    StringType _str_type;
    VectorType _vec_type;
};

//Реализации VectorType
void VectorType::push_back(TypeBase* _val){
    _value.push_back(Any(_val));
}

void VectorType::serialize(Buffer& buff) const{
        TypeBase::serialize(buff);
        write_value(buff, _value.size());
        for (auto i:_value) {
            i.serialize(buff);
        }
    }

Buffer::const_iterator VectorType::deserialize(Buffer::const_iterator begin, Buffer::const_iterator end){
        uint64_t size = read_value<uint64_t>(begin, end);
        for (int i = 0; i<size; i++) {
            Any any{};
            begin = any.deserialize(begin, end);
            _value.push_back(any);
        }
        return begin;
    }

VectorType::VectorType(std::vector<TypeBase*> other):XType<std::vector<Any>>(TypeId::Vector){
    for (auto item : other) {
        push_back(item);
    }
}

//основной сериализатор
class Serializator {
public:
    template<typename Arg>
    typename std::enable_if<
    std::is_same_v<Arg, IntegerType> || 
    std::is_same_v<Arg, FloatType> ||
    std::is_same_v<Arg, VectorType>||
    std::is_same_v<Arg, Any>||
    std::is_same_v<Arg, StringType>, void
    >::type
    push(Arg& _val){
        _storage.push_back(Any(_val));
    }

    Buffer serialize() const{
        Buffer buff;
        write_value(buff, _storage.size());
        for (auto item : _storage) {
            item.serialize(buff);
        }
        return buff;
    }

    static std::vector<Any> deserialize(const Buffer& _val){
        std::vector<Any> res;
        auto begin = _val.begin();
        uint64_t count = read_value<uint64_t>(begin, _val.end());
        for (int i = 0; i<count; i++) {
            Any item{};
            begin = item.deserialize(begin, _val.end());
            res.push_back(Any{item});
        }
        return res;
    }

    const std::vector<Any>& getStorage() const{
        return _storage;
    }

private:
    std::vector<Any> _storage;
};


bool init_test(){
    //Проверка на инициализацию
    IntegerType t1{5};
    FloatType t2{2.5};
    StringType t3{"qwerty"};
    //Доп. проверка на инициализацию вектора массивом
    std::vector<TypeBase*> values = {&t1, &t2, &t3};
    VectorType t4{values};
    return values.size() == t4.get_value().size();
}

template<typename Arg,typename T>
    typename std::enable_if<
    std::is_same_v<T, IntegerType> || 
    std::is_same_v<T, FloatType> ||
    std::is_same_v<T, VectorType>||
    std::is_same_v<T, StringType>, bool
    >::type
equals_test(Arg arg){
    //Проверка на сравнения 2-х типов
    T t1{arg};
    T t2{arg};
    return t1 == t2;
}


int main() {

    std::cout<< "Check inits\t" << init_test() << '\n';

    
    std::cout<< "Check string equals\t" << equals_test<std::string, StringType>("qwerty") << '\n';
    std::cout<< "Check float equals\t" << equals_test<double, FloatType>(5.5) << '\n';
    std::cout<< "Check integer equals\t" << equals_test<uint64_t, IntegerType>(100500) << '\n';
    IntegerType i1{5};
    std::cout<< "Check vector and any equals\t" << equals_test<std::vector<Any>, VectorType>(std::vector<Any>{Any{&i1}}) << '\n';
    
    //Старый код. Не модифицировал

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